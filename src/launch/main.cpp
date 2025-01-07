#include "main.hpp"
#include <cerrno>

#include <fcntl.h>
#include <qcoreapplication.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qtenvironmentvariables.h>
#include <unistd.h>

#include "../core/instanceinfo.hpp"
#include "../core/logging.hpp"
#include "../core/paths.hpp"
#include "build.hpp"
#include "launch_p.hpp"

#if CRASH_REPORTER
#include "../crash/main.hpp"
#endif

namespace qs::launch {

namespace {

void checkCrashRelaunch(char** argv, QCoreApplication* coreApplication) {
#if CRASH_REPORTER
	auto lastInfoFdStr = qEnvironmentVariable("__QUICKSHELL_CRASH_INFO_FD");

	if (!lastInfoFdStr.isEmpty()) {
		auto lastInfoFd = lastInfoFdStr.toInt();

		QFile file;
		file.open(lastInfoFd, QFile::ReadOnly, QFile::AutoCloseHandle);
		file.seek(0);

		auto ds = QDataStream(&file);
		RelaunchInfo info;
		ds >> info;

		LogManager::init(
		    !info.noColor,
		    info.timestamp,
		    info.sparseLogsOnly,
		    info.defaultLogLevel,
		    info.logRules
		);

		qCritical().nospace() << "Quickshell has crashed under pid "
		                      << qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt()
		                      << " (Coredumps will be available under that pid.)";

		qCritical() << "Further crash information is stored under"
		            << QsPaths::crashDir(info.instance.instanceId).path();

		if (info.instance.launchTime.msecsTo(QDateTime::currentDateTime()) < 10000) {
			qCritical() << "Quickshell crashed within 10 seconds of launching. Not restarting to avoid "
			               "a crash loop.";
			exit(-1); // NOLINT
		} else {
			qCritical() << "Quickshell has been restarted.";

			launch({.configPath = info.instance.configPath}, argv, coreApplication);
		}
	}
#endif
}

} // namespace

int DAEMON_PIPE = -1; // NOLINT

void exitDaemon(int code) {
	if (DAEMON_PIPE == -1) return;

	if (write(DAEMON_PIPE, &code, sizeof(int)) == -1) {
		qCritical().nospace() << "Failed to write daemon exit command with error code " << errno << ": "
		                      << qt_error_string();
	}

	close(DAEMON_PIPE);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	if (open("/dev/null", O_RDONLY) != STDIN_FILENO) { // NOLINT
		qFatal() << "Failed to open /dev/null on stdin";
	}

	if (open("/dev/null", O_WRONLY) != STDOUT_FILENO) { // NOLINT
		qFatal() << "Failed to open /dev/null on stdout";
	}

	if (open("/dev/null", O_WRONLY) != STDERR_FILENO) { // NOLINT
		qFatal() << "Failed to open /dev/null on stderr";
	}
}

int main(int argc, char** argv) {
	QCoreApplication::setApplicationName("quickshell");

#if CRASH_REPORTER
	qsCheckCrash(argc, argv);
#endif

	auto qArgC = 1;
	auto* coreApplication = new QCoreApplication(qArgC, argv);

	checkCrashRelaunch(argv, coreApplication);
	auto code = runCommand(argc, argv, coreApplication);

	exitDaemon(code);
	return code;
}

} // namespace qs::launch
