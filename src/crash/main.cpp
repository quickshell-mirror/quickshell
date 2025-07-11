#include "main.hpp"
#include <cerrno>
#include <cstdlib>

#include <qapplication.h>
#include <qconfig.h>
#include <qcoreapplication.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qfile.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <qtversion.h>
#include <sys/sendfile.h>
#include <sys/types.h>

#include "../core/instanceinfo.hpp"
#include "../core/logcat.hpp"
#include "../core/logging.hpp"
#include "../core/paths.hpp"
#include "build.hpp"
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
	auto dumpDupStatus = tryDup(dumpFd, crashDir.filePath("minidump.dmp.log"));
	if (dumpDupStatus != 0) {
		qCCritical(logCrashReporter) << "Failed to write minidump:" << dumpDupStatus;
	}

	qCDebug(logCrashReporter) << "Saving log from fd" << logFd;
	auto logDupStatus = tryDup(logFd, crashDir.filePath("log.qslog.log"));
	if (logDupStatus != 0) {
		qCCritical(logCrashReporter) << "Failed to save log:" << logDupStatus;
	}

	auto copyBinStatus = 0;
	if (!DISTRIBUTOR_DEBUGINFO_AVAILABLE) {
		qCDebug(logCrashReporter) << "Copying binary to crash folder";
		if (!QFile(QCoreApplication::applicationFilePath()).copy(crashDir.filePath("executable.txt"))) {
			copyBinStatus = 1;
			qCCritical(logCrashReporter) << "Failed to copy binary.";
		}
	}

	{
		auto extraInfoFile = QFile(crashDir.filePath("info.txt"));
		if (!extraInfoFile.open(QFile::WriteOnly)) {
			qCCritical(logCrashReporter) << "Failed to open crash info file for writing.";
		} else {
			auto stream = QTextStream(&extraInfoFile);
			stream << "===== Build Information =====\n";
			stream << "Git Revision: " << GIT_REVISION << '\n';
			stream << "Buildtime Qt Version: " << QT_VERSION_STR << "\n";
			stream << "Build Type: " << BUILD_TYPE << '\n';
			stream << "Compiler: " << COMPILER << '\n';
			stream << "Complie Flags: " << COMPILE_FLAGS << "\n\n";
			stream << "Build configuration:\n" << BUILD_CONFIGURATION << "\n";

			stream << "\n===== Runtime Information =====\n";
			stream << "Runtime Qt Version: " << qVersion() << '\n';
			stream << "Crashed process ID: " << crashProc << '\n';
			stream << "Run ID: " << instance.instanceId << '\n';
			stream << "Shell ID: " << instance.shellId << '\n';
			stream << "Config Path: " << instance.configPath << '\n';

			stream << "\n===== Report Integrity =====\n";
			stream << "Minidump save status: " << dumpDupStatus << '\n';
			stream << "Log save status: " << logDupStatus << '\n';
			stream << "Binary copy status: " << copyBinStatus << '\n';

			stream << "\n===== System Information =====\n\n";
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
				stream << '\n' << lsbReleaseFile.readAll();
				lsbReleaseFile.close();
			} else {
				stream << "FAILED TO OPEN\n";
			}

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

	auto app = QApplication(argc, argv);
	QApplication::setDesktopFileName("org.quickshell");

	auto crashDir = QsPaths::crashDir(info.instance.instanceId);

	qCInfo(logCrashReporter) << "Starting crash reporter...";

	recordCrashInfo(crashDir, info.instance);

	auto gui = CrashReporterGui(crashDir.path(), crashProc);
	gui.show();
	exit(QApplication::exec()); // NOLINT
}
