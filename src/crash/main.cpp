#include "main.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <cpptrace/basic.hpp>
#include <cpptrace/formatting.hpp>
#include <qapplication.h>
#include <qcoreapplication.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qfile.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <qtypes.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <unistd.h>

#include "../core/debuginfo.hpp"
#include "../core/instanceinfo.hpp"
#include "../core/logcat.hpp"
#include "../core/logging.hpp"
#include "../core/logging_p.hpp"
#include "../core/paths.hpp"
#include "../core/ringbuf.hpp"
#include "interface.hpp"

namespace {

QS_LOGGING_CATEGORY(logCrashReporter, "quickshell.crashreporter", QtWarningMsg);

int tryDup(int fd, const QString& path) {
	QFile sourceFile;
	if (!sourceFile.open(fd, QFile::ReadOnly, QFile::AutoCloseHandle)) {
		qCCritical(logCrashReporter) << "Failed to open source fd for duplication.";
		return 1;
	}

	auto destFile = QFile(path);
	if (!destFile.open(QFile::WriteOnly)) {
		qCCritical(logCrashReporter) << "Failed to open dest file for duplication.";
		return 2;
	}

	auto size = sourceFile.size();
	off_t offset = 0;
	ssize_t count = 0;

	sourceFile.seek(0);

	while (count != size) {
		auto r = sendfile(destFile.handle(), sourceFile.handle(), &offset, sourceFile.size());
		if (r == -1) {
			qCCritical(logCrashReporter).nospace()
			    << "Failed to duplicate fd " << fd << " with error code " << errno
			    << ". Error: " << qt_error_string();
			return 3;
		} else {
			count += r;
		}
	}

	return 0;
}

QString readRecentLogs(int logFd, int maxLines, qint64 maxAgeSecs) {
	QFile file;
	if (!file.open(logFd, QFile::ReadOnly, QFile::AutoCloseHandle)) {
		return QStringLiteral("(failed to open log fd)\n");
	}

	file.seek(0);

	qs::log::EncodedLogReader reader;
	reader.setDevice(&file);

	bool readable = false;
	quint8 logVersion = 0;
	quint8 readerVersion = 0;
	if (!reader.readHeader(&readable, &logVersion, &readerVersion) || !readable) {
		return QStringLiteral("(failed to read log header)\n");
	}

	// Read all messages, keeping last maxLines in a ring buffer
	auto tail = RingBuffer<qs::log::LogMessage>(maxLines);
	qs::log::LogMessage message;
	while (reader.read(&message)) {
		tail.emplace(message);
	}

	if (tail.size() == 0) {
		return QStringLiteral("(no logs)\n");
	}

	// Filter to only messages within maxAgeSecs of the newest message
	auto cutoff = tail.at(0).time.addSecs(-maxAgeSecs);

	QString result;
	auto stream = QTextStream(&result);
	for (auto i = tail.size() - 1; i != -1; i--) {
		if (tail.at(i).time < cutoff) continue;
		qs::log::LogMessage::formatMessage(stream, tail.at(i), false, true);
		stream << '\n';
	}

	if (result.isEmpty()) {
		return QStringLiteral("(no recent logs)\n");
	}

	return result;
}

cpptrace::stacktrace resolveStacktrace(int dumpFd) {
	QFile sourceFile;
	if (!sourceFile.open(dumpFd, QFile::ReadOnly, QFile::AutoCloseHandle)) {
		qCCritical(logCrashReporter) << "Failed to open trace memfd.";
		return {};
	}

	sourceFile.seek(0);
	auto data = sourceFile.readAll();

	auto frameCount = static_cast<size_t>(data.size()) / sizeof(cpptrace::safe_object_frame);
	if (frameCount == 0) return {};

	const auto* frames = reinterpret_cast<const cpptrace::safe_object_frame*>(data.constData());

	cpptrace::object_trace objectTrace;
	for (size_t i = 0; i < frameCount; i++) {
		objectTrace.frames.push_back(frames[i].resolve()); // NOLINT
	}

	return objectTrace.resolve();
}

void recordCrashInfo(const QDir& crashDir, const InstanceInfo& instance) {
	qCDebug(logCrashReporter) << "Recording crash information at" << crashDir.path();

	if (!crashDir.mkpath(".")) {
		qCCritical(logCrashReporter) << "Failed to create folder" << crashDir
		                             << "to save crash information.";
		return;
	}

	auto crashProc = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt();
	auto crashSignal = qEnvironmentVariable("__QUICKSHELL_CRASH_SIGNAL").toInt();
	auto dumpFd = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_FD").toInt();
	auto logFd = qEnvironmentVariable("__QUICKSHELL_CRASH_LOG_FD").toInt();

	qCDebug(logCrashReporter) << "Resolving stacktrace from fd" << dumpFd;
	auto stacktrace = resolveStacktrace(dumpFd);

	qCDebug(logCrashReporter) << "Reading recent log lines from fd" << logFd;
	auto logDupFd = dup(logFd);
	auto recentLogs = readRecentLogs(logFd, 100, 10);

	qCDebug(logCrashReporter) << "Saving log from fd" << logDupFd;
	auto logDupStatus = tryDup(logDupFd, crashDir.filePath("log.qslog.log"));
	if (logDupStatus != 0) {
		qCCritical(logCrashReporter) << "Failed to save log:" << logDupStatus;
	}

	{
		auto extraInfoFile = QFile(crashDir.filePath("report.txt"));
		if (!extraInfoFile.open(QFile::WriteOnly)) {
			qCCritical(logCrashReporter) << "Failed to open crash info file for writing.";
		} else {
			auto stream = QTextStream(&extraInfoFile);
			stream << qs::debuginfo::combinedInfo();

			stream << "\n===== Instance Information =====\n";
			stream << "Signal: " << strsignal(crashSignal) << " (" << crashSignal << ")\n"; // NOLINT
			stream << "Crashed process ID: " << crashProc << '\n';
			stream << "Run ID: " << instance.instanceId << '\n';
			stream << "Shell ID: " << instance.shellId << '\n';
			stream << "Config Path: " << instance.configPath << '\n';

			stream << "\n===== Stacktrace =====\n";
			if (stacktrace.empty()) {
				stream << "(no trace available)\n";
			} else {
				auto formatter = cpptrace::formatter().header(std::string());
				auto traceStr = formatter.format(stacktrace);
				stream << QString::fromStdString(traceStr) << '\n';
			}

			stream << "\n===== Log Tail =====\n";
			stream << recentLogs;

			extraInfoFile.close();
		}
	}

	qCDebug(logCrashReporter) << "Recorded crash information.";
}

} // namespace

void qsCheckCrash(int argc, char** argv) {
	auto fd = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_FD");
	if (fd.isEmpty()) return;

	RelaunchInfo info;

	auto crashProc = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt();

	{
		auto infoFd = qEnvironmentVariable("__QUICKSHELL_CRASH_INFO_FD").toInt();

		QFile file;
		if (!file.open(infoFd, QFile::ReadOnly, QFile::AutoCloseHandle)) {
			qFatal() << "Failed to open instance info fd.";
		}

		file.seek(0);

		auto ds = QDataStream(&file);
		ds >> info;
	}

	LogManager::init(
	    !info.noColor,
	    info.timestamp,
	    info.sparseLogsOnly,
	    info.defaultLogLevel,
	    info.logRules
	);

	auto app = QApplication(argc, argv);
	QApplication::setDesktopFileName("org.quickshell");

	auto crashDir = QsPaths::crashDir(info.instance.instanceId);

	qCInfo(logCrashReporter) << "Starting crash reporter...";

	recordCrashInfo(crashDir, info.instance);

	auto gui = CrashReporterGui(crashDir.path(), crashProc);
	gui.show();
	exit(QApplication::exec()); // NOLINT
}
