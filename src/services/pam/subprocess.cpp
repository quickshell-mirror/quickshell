#include "subprocess.hpp"
#include <array>
#include <csignal>
#include <ostream>
#include <string>

#include <fcntl.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <sched.h>
#ifdef __FreeBSD__
#include <security/pam_types.h>
#else
#include <security/_pam_types.h>
#endif
#include <security/pam_appl.h>
#include <unistd.h>

#include "conversation.hpp"
#include "ipc.hpp"

namespace {

// Handlers installed by the shell, such as its crash handler, must not run in the subprocess.
// Reset them as exec would, leaving ignored signals ignored.
void resetSignalHandlers() {
	// NOLINTBEGIN (misc-include-cleaner)
	for (auto sig = 1; sig < NSIG; sig++) {
		struct sigaction action {};
		if (sigaction(sig, nullptr, &action) == -1) continue;
		if (action.sa_handler != SIG_DFL && action.sa_handler != SIG_IGN) signal(sig, SIG_DFL);
	}
	// NOLINTEND (misc-include-cleaner)
}

#ifdef __FreeBSD__
void onLifelineClosed(int /*sig*/) { _exit(1); }
#endif

// Pipes raise SIGIO for an O_ASYNC reader when their last writer closes (Linux pipe_release,
// FreeBSD pipeclose), so this kills the subprocess once every copy of the lifeline's write end
// is gone. Unlike a parent death signal, that also happens when the shell relaunches in place
// after a crash, as exec closes the shell's copy. Writing to the lifeline also triggers it.
void armLifeline(int fd) {
	// NOLINTBEGIN (misc-include-cleaner)
	fcntl(fd, F_SETOWN, getpid());

#ifdef __FreeBSD__
	// There is no F_SETSIG, and SIGIO is ignored by default.
	struct sigaction action {};
	action.sa_handler = &onLifelineClosed;
	sigemptyset(&action.sa_mask);
	sigaction(SIGIO, &action, nullptr);

	sigset_t mask {};
	sigemptyset(&mask);
	sigaddset(&mask, SIGIO);
	sigprocmask(SIG_UNBLOCK, &mask, nullptr);
#else
	// Unlike SIGIO, a pam module cannot catch, block or ignore this.
	fcntl(fd, F_SETSIG, SIGKILL);
#endif

	auto flags = fcntl(fd, F_GETFL);
	if (flags != -1) fcntl(fd, F_SETFL, flags | O_ASYNC);
	// NOLINTEND (misc-include-cleaner)
}

} // namespace

pid_t PamConversation::createSubprocess(
    PamIpcPipes* pipes,
    int* lifelineFd,
    const QString& configDir,
    const QString& config,
    const QString& user
) {
	auto toSubprocess = std::array<int, 2>();
	auto fromSubprocess = std::array<int, 2>();
	auto lifeline = std::array<int, 2>();

	// The lifeline must be CLOEXEC so exec drops the shell's end, including the crash handler's
	// in-place relaunch. Forks that never exec (later subprocesses, the crash handler's coredump
	// child) keep a copy until they exit.
	if (pipe(toSubprocess.data()) == -1 || pipe(fromSubprocess.data()) == -1
	    || pipe2(lifeline.data(), O_CLOEXEC) == -1)
	{
		qCDebug(logPam) << "Failed to create pipes for subprocess.";
		return 0;
	}

	auto* configDirF = strdup(configDir.toStdString().c_str()); // NOLINT (include)
	auto* configF = strdup(config.toStdString().c_str());       // NOLINT (include)
	auto* userF = strdup(user.toStdString().c_str());           // NOLINT (include)
	auto log = logPam().isDebugEnabled();

	auto pid = fork();

	if (pid < 0) {
		qCDebug(logPam) << "Failed to fork for subprocess.";
	} else if (pid == 0) {
		resetSignalHandlers();

		// Armed before our copy of the write end is closed, so if the shell is already gone,
		// closing it triggers the lifeline.
		armLifeline(lifeline[0]);
		close(lifeline[1]); // close w

		close(toSubprocess[1]);   // close w
		close(fromSubprocess[0]); // close r

		{
			auto subprocess = PamSubprocess(log, toSubprocess[0], fromSubprocess[1]);
			auto code = subprocess.exec(configDirF, configF, userF);
			subprocess.sendCode(code);
		}

		free(configDirF); // NOLINT
		free(configF);    // NOLINT
		free(userF);      // NOLINT

		// do not do cleanup that may affect the parent
		_exit(0);
	} else {
		close(toSubprocess[0]);   // close r
		close(fromSubprocess[1]); // close w
		close(lifeline[0]);       // close r

		pipes->fdIn = fromSubprocess[0];
		pipes->fdOut = toSubprocess[1];
		*lifelineFd = lifeline[1];

		free(configDirF); // NOLINT
		free(configF);    // NOLINT
		free(userF);      // NOLINT

		return pid;
	}

	return -1; // should never happen but lint
}

PamIpcExitCode PamSubprocess::exec(const char* configDir, const char* config, const char* user) {
	logIf(this->log) << "Waiting for parent confirmation..." << std::endl;

	auto conv = pam_conv {
	    .conv = &PamSubprocess::conversation,
	    .appdata_ptr = this,
	};

	pam_handle_t* handle = nullptr;

	logIf(this->log) << "Starting pam session for user \"" << user << "\" with config \"" << config
	                 << "\" in dir \"" << configDir << "\"" << std::endl;

#ifdef __FreeBSD__
	auto result = pam_start(config, user, &conv, &handle);
#else
	auto result = pam_start_confdir(config, user, &conv, configDir, &handle);
#endif

	if (result != PAM_SUCCESS) {
		logIf(true) << "Unable to start pam conversation with error \"" << pam_strerror(handle, result)
		            << "\" (code " << result << ")" << std::endl;
		return PamIpcExitCode::StartFailed;
	}

	result = pam_authenticate(handle, 0);
	PamIpcExitCode code = PamIpcExitCode::OtherError;

	switch (result) {
	case PAM_SUCCESS:
		logIf(this->log) << "Authenticated successfully." << std::endl;
		code = PamIpcExitCode::Success;
		break;
	case PAM_AUTH_ERR:
		logIf(this->log) << "Failed to authenticate." << std::endl;
		code = PamIpcExitCode::AuthFailed;
		break;
	case PAM_MAXTRIES:
		logIf(this->log) << "Failed to authenticate due to hitting max tries." << std::endl;
		code = PamIpcExitCode::MaxTries;
		break;
	default:
		logIf(true) << "Error while authenticating: \"" << pam_strerror(handle, result) << "\" (code "
		            << result << ")" << std::endl;
		code = PamIpcExitCode::PamError;
		break;
	}

	result = pam_end(handle, result);
	if (result != PAM_SUCCESS) {
		logIf(true) << "Error in pam_end: \"" << pam_strerror(handle, result) << "\" (code " << result
		            << ")" << std::endl;
	}

	return code;
}

void PamSubprocess::sendCode(PamIpcExitCode code) {
	{
		auto eventType = PamIpcEvent::Exit;
		auto ok = this->pipes.writeBytes(reinterpret_cast<char*>(&eventType), sizeof(PamIpcEvent));

		if (!ok) goto fail;

		ok = this->pipes.writeBytes(reinterpret_cast<char*>(&code), sizeof(PamIpcExitCode));

		if (!ok) goto fail;

		return;
	}

fail:
	_exit(1);
}

int PamSubprocess::conversation(
    int msgCount,
    const pam_message** msgArray,
    pam_response** responseArray,
    void* appdata
) {
	auto* delegate = static_cast<PamSubprocess*>(appdata);

	// freed by libc so must be alloc'd by it.
	auto* responses = static_cast<pam_response*>(calloc(msgCount, sizeof(pam_response))); // NOLINT

	for (auto i = 0; i < msgCount; i++) {
		const auto* message = msgArray[i]; // NOLINT
		auto& response = responses[i];     // NOLINT

		auto msgString = std::string(message->msg);
		auto req = PamIpcRequestFlags {
		    .echo = message->msg_style != PAM_PROMPT_ECHO_OFF,
		    .error = message->msg_style == PAM_ERROR_MSG,
		    .responseRequired =
		        message->msg_style == PAM_PROMPT_ECHO_OFF || message->msg_style == PAM_PROMPT_ECHO_ON,
		};

		logIf(delegate->log) << "Relaying pam message: \"" << msgString << "\" echo: " << req.echo
		                     << " error: " << req.error << " responseRequired: " << req.responseRequired
		                     << std::endl;

		auto eventType = PamIpcEvent::Request;
		auto ok = delegate->pipes.writeBytes(reinterpret_cast<char*>(&eventType), sizeof(PamIpcEvent));

		if (!ok) goto fail;

		ok =
		    delegate->pipes.writeBytes(reinterpret_cast<const char*>(&req), sizeof(PamIpcRequestFlags));

		if (!ok) goto fail;
		if (!delegate->pipes.writeString(msgString)) goto fail;

		if (req.responseRequired) {
			auto ok = false;
			auto resp = delegate->pipes.readString(&ok);
			if (!ok) _exit(static_cast<int>(PamIpcExitCode::OtherError));
			logIf(delegate->log) << "Got response for request.";

			response.resp = strdup(resp.c_str()); // NOLINT (include)
		}
	}

	*responseArray = responses;
	return PAM_SUCCESS;

fail:
	free(responses); // NOLINT
	_exit(1);
}
