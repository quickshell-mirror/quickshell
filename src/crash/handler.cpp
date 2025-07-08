#include "handler.hpp"
#include <array>
#include <cstdio>
#include <cstring>

#include <bits/types/sigset_t.h>
#include <breakpad/client/linux/handler/exception_handler.h>
#include <breakpad/client/linux/handler/minidump_descriptor.h>
#include <breakpad/common/linux/linux_libc_support.h>
#include <qdatastream.h>
#include <qfile.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../core/instanceinfo.hpp"
#include "../core/logcat.hpp"

extern char** environ; // NOLINT

using namespace google_breakpad;

namespace qs::crash {

namespace {
QS_LOGGING_CATEGORY(logCrashHandler, "quickshell.crashhandler", QtWarningMsg);
}

struct CrashHandlerPrivate {
	ExceptionHandler* exceptionHandler = nullptr;
	int minidumpFd = -1;
	int infoFd = -1;

	static bool minidumpCallback(const MinidumpDescriptor& descriptor, void* context, bool succeeded);
};

CrashHandler::CrashHandler(): d(new CrashHandlerPrivate()) {}

void CrashHandler::init() {
	// MinidumpDescriptor has no move constructor and the copy constructor breaks fds.
	auto createHandler = [this](const MinidumpDescriptor& desc) {
		this->d->exceptionHandler = new ExceptionHandler(
		    desc,
		    nullptr,
		    &CrashHandlerPrivate::minidumpCallback,
		    this->d,
		    true,
		    -1
		);
	};

	qCDebug(logCrashHandler) << "Starting crash handler...";

	this->d->minidumpFd = memfd_create("quickshell:minidump", MFD_CLOEXEC);

	if (this->d->minidumpFd == -1) {
		qCCritical(logCrashHandler
		) << "Failed to allocate minidump memfd, minidumps will be saved in the working directory.";
		createHandler(MinidumpDescriptor("."));
	} else {
		qCDebug(logCrashHandler) << "Created memfd" << this->d->minidumpFd
		                         << "for holding possible minidumps.";
		createHandler(MinidumpDescriptor(this->d->minidumpFd));
	}

	qCInfo(logCrashHandler) << "Crash handler initialized.";
}

void CrashHandler::setRelaunchInfo(const RelaunchInfo& info) {
	this->d->infoFd = memfd_create("quickshell:instance_info", MFD_CLOEXEC);

	if (this->d->infoFd == -1) {
		qCCritical(logCrashHandler
		) << "Failed to allocate instance info memfd, crash recovery will not work.";
		return;
	}

	QFile file;
	file.open(this->d->infoFd, QFile::ReadWrite);

	QDataStream ds(&file);
	ds << info;
	file.flush();

	qCDebug(logCrashHandler) << "Stored instance info in memfd" << this->d->infoFd;
}

CrashHandler::~CrashHandler() {
	delete this->d->exceptionHandler;
	delete this->d;
}

bool CrashHandlerPrivate::minidumpCallback(
    const MinidumpDescriptor& /*descriptor*/,
    void* context,
    bool /*success*/
) {
	// A fork that just dies to ensure the coredump is caught by the system.
	auto coredumpPid = fork();

	if (coredumpPid == 0) {
		return false;
	}

	auto* self = static_cast<CrashHandlerPrivate*>(context);

	auto exe = std::array<char, 4096>();
	if (readlink("/proc/self/exe", exe.data(), exe.size() - 1) == -1) {
		perror("Failed to find crash reporter executable.\n");
		_exit(-1);
	}

	auto arg = std::array<char*, 2> {exe.data(), nullptr};

	auto env = std::array<char*, 4096>();
	auto envi = 0;

	auto infoFd = dup(self->infoFd);
	auto infoFdStr = std::array<char, 38>();
	memcpy(infoFdStr.data(), "__QUICKSHELL_CRASH_INFO_FD=-1" /*\0*/, 30);
	if (infoFd != -1) my_uitos(&infoFdStr[27], infoFd, 10);
	env[envi++] = infoFdStr.data();

	auto corePidStr = std::array<char, 39>();
	memcpy(corePidStr.data(), "__QUICKSHELL_CRASH_DUMP_PID=-1" /*\0*/, 31);
	if (coredumpPid != -1) my_uitos(&corePidStr[28], coredumpPid, 10);
	env[envi++] = corePidStr.data();

	auto populateEnv = [&]() {
		auto senvi = 0;
		while (envi != 4095) {
			auto var = environ[senvi++]; // NOLINT
			if (var == nullptr) break;
			env[envi++] = var;
		}

		env[envi] = nullptr;
	};

	sigset_t sigset;
	sigemptyset(&sigset);                       // NOLINT (include)
	sigprocmask(SIG_SETMASK, &sigset, nullptr); // NOLINT

	auto pid = fork();

	if (pid == -1) {
		perror("Failed to fork and launch crash reporter.\n");
		return false;
	} else if (pid == 0) {
		// dup to remove CLOEXEC
		// if already -1 will return -1
		auto dumpFd = dup(self->minidumpFd);
		auto logFd = dup(CrashInfo::INSTANCE.logFd);

		// allow up to 10 digits, which should never happen
		auto dumpFdStr = std::array<char, 38>();
		auto logFdStr = std::array<char, 37>();

		memcpy(dumpFdStr.data(), "__QUICKSHELL_CRASH_DUMP_FD=-1" /*\0*/, 30);
		memcpy(logFdStr.data(), "__QUICKSHELL_CRASH_LOG_FD=-1" /*\0*/, 29);

		if (dumpFd != -1) my_uitos(&dumpFdStr[27], dumpFd, 10);
		if (logFd != -1) my_uitos(&logFdStr[26], logFd, 10);

		env[envi++] = dumpFdStr.data();
		env[envi++] = logFdStr.data();

		populateEnv();
		execve(exe.data(), arg.data(), env.data());

		perror("Failed to launch crash reporter.\n");
		_exit(-1);
	} else {
		populateEnv();
		execve(exe.data(), arg.data(), env.data());

		perror("Failed to relaunch quickshell.\n");
		_exit(-1);
	}

	return false; // should make sure it hits the system coredump handler
}

} // namespace qs::crash
