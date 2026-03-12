#include "handler.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>

#include <cpptrace/basic.hpp>
#include <cpptrace/forward.hpp>
#include <qdatastream.h>
#include <qfile.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../core/instanceinfo.hpp"
#include "../core/logcat.hpp"

extern char** environ; // NOLINT

namespace qs::crash {

namespace {

QS_LOGGING_CATEGORY(logCrashHandler, "quickshell.crashhandler", QtWarningMsg);

void writeEnvInt(char* buf, const char* name, int value) {
	// NOLINTBEGIN (cppcoreguidelines-pro-bounds-pointer-arithmetic)
	while (*name != '\0') *buf++ = *name++;
	*buf++ = '=';

	if (value < 0) {
		*buf++ = '-';
		value = -value;
	}

	if (value == 0) {
		*buf++ = '0';
		*buf = '\0';
		return;
	}

	auto* start = buf;
	while (value > 0) {
		*buf++ = static_cast<char>('0' + (value % 10));
		value /= 10;
	}

	*buf = '\0';
	std::reverse(start, buf);
	// NOLINTEND
}

void signalHandler(
    int sig,
    siginfo_t* /*info*/, // NOLINT (misc-include-cleaner)
    void*                /*context*/
) {
	if (CrashInfo::INSTANCE.traceFd != -1) {
		auto traceBuffer = std::array<cpptrace::frame_ptr, 1024>();
		auto frameCount = cpptrace::safe_generate_raw_trace(traceBuffer.data(), traceBuffer.size(), 1);

		for (size_t i = 0; i < static_cast<size_t>(frameCount); i++) {
			auto frame = cpptrace::safe_object_frame();
			cpptrace::get_safe_object_frame(traceBuffer[i], &frame);

			auto* wptr = reinterpret_cast<char*>(&frame);
			auto* end = wptr + sizeof(cpptrace::safe_object_frame); // NOLINT
			while (wptr != end) {
				auto r = write(CrashInfo::INSTANCE.traceFd, &frame, sizeof(cpptrace::safe_object_frame));
				if (r < 0 && errno == EINTR) continue;
				if (r <= 0) goto fail;
				wptr += r; // NOLINT
			}
		}

	fail:;
	}

	auto coredumpPid = fork();
	if (coredumpPid == 0) {
		// NOLINTBEGIN (misc-include-cleaner)
		sigset_t set;
		sigfillset(&set);
		sigprocmask(SIG_UNBLOCK, &set, nullptr);
		// NOLINTEND
		raise(sig);
		_exit(-1);
	}

	auto exe = std::array<char, 4096>();
	if (readlink("/proc/self/exe", exe.data(), exe.size() - 1) == -1) {
		perror("Failed to find crash reporter executable.\n");
		_exit(-1);
	}

	auto arg = std::array<char*, 2> {exe.data(), nullptr};

	auto env = std::array<char*, 4096>();
	auto envi = 0;

	// dup to remove CLOEXEC
	auto infoFdStr = std::array<char, 48>();
	writeEnvInt(infoFdStr.data(), "__QUICKSHELL_CRASH_INFO_FD", dup(CrashInfo::INSTANCE.infoFd));
	env[envi++] = infoFdStr.data();

	auto corePidStr = std::array<char, 48>();
	writeEnvInt(corePidStr.data(), "__QUICKSHELL_CRASH_DUMP_PID", coredumpPid);
	env[envi++] = corePidStr.data();

	auto sigStr = std::array<char, 48>();
	writeEnvInt(sigStr.data(), "__QUICKSHELL_CRASH_SIGNAL", sig);
	env[envi++] = sigStr.data();

	auto populateEnv = [&]() {
		auto senvi = 0;
		while (envi != 4095) {
			auto var = environ[senvi++]; // NOLINT
			if (var == nullptr) break;
			env[envi++] = var;
		}

		env[envi] = nullptr;
	};

	auto pid = fork();

	if (pid == -1) {
		perror("Failed to fork and launch crash reporter.\n");
		_exit(-1);
	} else if (pid == 0) {

		// dup to remove CLOEXEC
		auto dumpFdStr = std::array<char, 48>();
		auto logFdStr = std::array<char, 48>();
		writeEnvInt(dumpFdStr.data(), "__QUICKSHELL_CRASH_DUMP_FD", dup(CrashInfo::INSTANCE.traceFd));
		writeEnvInt(logFdStr.data(), "__QUICKSHELL_CRASH_LOG_FD", dup(CrashInfo::INSTANCE.logFd));

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
}

} // namespace

void CrashHandler::init() {
	qCDebug(logCrashHandler) << "Starting crash handler...";

	CrashInfo::INSTANCE.traceFd = memfd_create("quickshell:trace", MFD_CLOEXEC);

	if (CrashInfo::INSTANCE.traceFd == -1) {
		qCCritical(logCrashHandler) << "Failed to allocate trace memfd, stack traces will not be "
		                               "available in crash reports.";
	} else {
		qCDebug(logCrashHandler) << "Created memfd" << CrashInfo::INSTANCE.traceFd
		                         << "for holding possible stack traces.";
	}

	{
		// Preload anything dynamically linked to avoid malloc etc in the dynamic loader.
		// See cpptrace documentation for more information.
		auto buffer = std::array<cpptrace::frame_ptr, 10>();
		cpptrace::safe_generate_raw_trace(buffer.data(), buffer.size());
		auto frame = cpptrace::safe_object_frame();
		cpptrace::get_safe_object_frame(buffer[0], &frame);
	}

	// NOLINTBEGIN (misc-include-cleaner)

	// Set up alternate signal stack for stack overflow handling
	auto ss = stack_t();
	ss.ss_sp = new char[SIGSTKSZ];
	ss.ss_size = SIGSTKSZ;
	ss.ss_flags = 0;
	sigaltstack(&ss, nullptr);

	// Install signal handlers
	struct sigaction sa {};
	sa.sa_sigaction = &signalHandler;
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGABRT, &sa, nullptr);
	sigaction(SIGFPE, &sa, nullptr);
	sigaction(SIGILL, &sa, nullptr);
	sigaction(SIGBUS, &sa, nullptr);
	sigaction(SIGTRAP, &sa, nullptr);

	// NOLINTEND (misc-include-cleaner)

	qCInfo(logCrashHandler) << "Crash handler initialized.";
}

void CrashHandler::setRelaunchInfo(const RelaunchInfo& info) {
	CrashInfo::INSTANCE.infoFd = memfd_create("quickshell:instance_info", MFD_CLOEXEC);

	if (CrashInfo::INSTANCE.infoFd == -1) {
		qCCritical(
		    logCrashHandler
		) << "Failed to allocate instance info memfd, crash recovery will not work.";
		return;
	}

	QFile file;

	if (!file.open(CrashInfo::INSTANCE.infoFd, QFile::ReadWrite)) {
		qCCritical(
		    logCrashHandler
		) << "Failed to open instance info memfd, crash recovery will not work.";
	}

	QDataStream ds(&file);
	ds << info;
	file.flush();

	qCDebug(logCrashHandler) << "Stored instance info in memfd" << CrashInfo::INSTANCE.infoFd;
}

} // namespace qs::crash
