#pragma once

#include <iostream> // NOLINT std::cout

#include <security/pam_appl.h>

#include "ipc.hpp"

// endls are intentional as it makes debugging much easier when the buffer actually flushes.
// NOLINTBEGIN
#define logIf(log)                                                                                 \
	if (log) std::cout << "quickshell.service.pam.subprocess: "
// NOLINTEND

class PamSubprocess {
public:
	explicit PamSubprocess(bool log, int fdIn, int fdOut): log(log), pipes(fdIn, fdOut) {}
	PamIpcExitCode exec(const char* configDir, const char* config, const char* user);
	void sendCode(PamIpcExitCode code);

private:
	static int conversation(
	    int msgCount,
	    const pam_message** msgArray,
	    pam_response** responseArray,
	    void* appdata
	);

	bool log;
	PamIpcPipes pipes;
};
