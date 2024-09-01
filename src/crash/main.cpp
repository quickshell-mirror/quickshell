#include "main.hpp"
#include <cerrno>
#include <cstdlib>

#include <qapplication.h>
#include <qconfig.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qfile.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <sys/sendfile.h>
#include <sys/types.h>

#include "../core/instanceinfo.hpp"
#include "../core/logging.hpp"
#include "../core/paths.hpp"
#include "build.hpp"
#include "interface.hpp"

Q_LOGGING_CATEGORY(logCrashReporter, "quickshell.crashreporter", QtWarningMsg);

void recordCrashInfo(const QDir& crashDir, const InstanceInfo& instance);

void qsCheckCrash(int argc, char** argv) {
	auto fd = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_FD");
	if (fd.isEmpty()) return;
	auto app = QApplication(argc, argv);

	RelaunchInfo info;

	auto crashProc = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt();

	{
		auto infoFd = qEnvironmentVariable("__QUICKSHELL_CRASH_INFO_FD").toInt();

		QFile file;
		file.open(infoFd, QFile::ReadOnly, QFile::AutoCloseHandle);
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

	auto crashDir = QsPaths::crashDir(info.instance.instanceId);

	qCInfo(logCrashReporter) << "Starting crash reporter...";

	recordCrashInfo(crashDir, info.instance);

	auto gui = CrashReporterGui(crashDir.path(), crashProc);
	gui.show();
	exit(QApplication::exec()); // NOLINT
}

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

void recordCrashInfo(const QDir& crashDir, const InstanceInfo& instance) {
	qCDebug(logCrashReporter) << "Recording crash information at" << crashDir.path();

	if (!crashDir.mkpath(".")) {
		qCCritical(logCrashReporter) << "Failed to create folder" << crashDir
		                             << "to save crash information.";
		return;
	}

	auto crashProc = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt();
	auto dumpFd = qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_FD").toInt();
	auto logFd = qEnvironmentVariable("__QUICKSHELL_CRASH_LOG_FD").toInt();

	qCDebug(logCrashReporter) << "Saving minidump from fd" << dumpFd;
	auto dumpDupStatus = tryDup(dumpFd, crashDir.filePath("minidump.dmp"));
	if (dumpDupStatus != 0) {
		qCCritical(logCrashReporter) << "Failed to write minidump:" << dumpDupStatus;
	}

	qCDebug(logCrashReporter) << "Saving log from fd" << logFd;
	auto logDupStatus = tryDup(logFd, crashDir.filePath("log.qslog"));
	if (logDupStatus != 0) {
		qCCritical(logCrashReporter) << "Failed to save log:" << logDupStatus;
	}

	{
		auto extraInfoFile = QFile(crashDir.filePath("info.txt"));
		if (!extraInfoFile.open(QFile::WriteOnly)) {
			qCCritical(logCrashReporter) << "Failed to open crash info file for writing.";
		} else {
			auto stream = QTextStream(&extraInfoFile);
			stream << "===== Quickshell Crash =====\n";
			stream << "Git Revision: " << GIT_REVISION << '\n';
			stream << "Crashed process ID: " << crashProc << '\n';
			stream << "Run ID: " << instance.instanceId << '\n';

			stream << "\n===== Shell Information =====\n";
			stream << "Shell ID: " << instance.shellId << '\n';
			stream << "Config Path: " << instance.configPath << '\n';

			stream << "\n===== Report Integrity =====\n";
			stream << "Minidump save status: " << dumpDupStatus << '\n';
			stream << "Log save status: " << logDupStatus << '\n';

			stream << "\n===== System Information =====\n";
			stream << "Qt Version: " << QT_VERSION_STR << "\n\n";

			stream << "/etc/os-release:";
			auto osReleaseFile = QFile("/etc/os-release");
			if (osReleaseFile.open(QFile::ReadOnly)) {
				stream << '\n' << osReleaseFile.readAll() << '\n';
				osReleaseFile.close();
			} else {
				stream << "FAILED TO OPEN\n";
			}

			stream << "/etc/lsb-release:";
			auto lsbReleaseFile = QFile("/etc/lsb-release");
			if (lsbReleaseFile.open(QFile::ReadOnly)) {
				stream << '\n' << lsbReleaseFile.readAll() << '\n';
				lsbReleaseFile.close();
			} else {
				stream << "FAILED TO OPEN\n";
			}

			extraInfoFile.close();
		}
	}

	qCDebug(logCrashReporter) << "Recorded crash information.";
}
