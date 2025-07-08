#include "conversation.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <sys/wait.h>

#include "../../core/logcat.hpp"
#include "ipc.hpp"

QS_LOGGING_CATEGORY(logPam, "quickshell.service.pam", QtWarningMsg);

QString PamError::toString(PamError::Enum value) {
	switch (value) {
	case StartFailed: return "Failed to start the PAM session";
	case TryAuthFailed: return "Failed to try authenticating";
	case InternalError: return "Internal error occurred";
	default: return "Invalid error";
	}
}

QString PamResult::toString(PamResult::Enum value) {
	switch (value) {
	case Success: return "Success";
	case Failed: return "Failed";
	case Error: return "Error occurred while authenticating";
	case MaxTries: return "The authentication method has no more attempts available";
	default: return "Invalid result";
	}
}

PamConversation::~PamConversation() { this->abort(); }

void PamConversation::start(const QString& configDir, const QString& config, const QString& user) {
	this->childPid = PamConversation::createSubprocess(&this->pipes, configDir, config, user);
	if (this->childPid == 0) {
		qCCritical(logPam) << "Failed to create pam subprocess.";
		emit this->error(PamError::InternalError);
		return;
	}

	QObject::connect(&this->notifier, &QSocketNotifier::activated, this, &PamConversation::onMessage);
	this->notifier.setSocket(this->pipes.fdIn);
	this->notifier.setEnabled(true);
}

void PamConversation::abort() {
	if (this->childPid != 0) {
		qCDebug(logPam) << "Killing subprocess for" << this;
		kill(this->childPid, SIGKILL); // NOLINT (include)
		waitpid(this->childPid, nullptr, 0);
		this->childPid = 0;
	}
}

void PamConversation::internalError() {
	if (this->childPid != 0) {
		qCDebug(logPam) << "Killing subprocess for" << this;
		kill(this->childPid, SIGKILL); // NOLINT (include)
		waitpid(this->childPid, nullptr, 0);
		this->childPid = 0;
		emit this->error(PamError::InternalError);
	}
}

void PamConversation::respond(const QString& response) {
	qCDebug(logPam) << "Sending response for" << this;
	if (!this->pipes.writeString(response.toStdString())) {
		qCCritical(logPam) << "Failed to write response to subprocess.";
		this->internalError();
	}
}

void PamConversation::onMessage() {
	{
		qCDebug(logPam) << "Got message from subprocess.";

		auto type = PamIpcEvent::Exit;

		auto ok = this->pipes.readBytes(reinterpret_cast<char*>(&type), sizeof(PamIpcEvent));

		if (!ok) goto fail;

		if (type == PamIpcEvent::Exit) {
			auto code = PamIpcExitCode::OtherError;

			ok = this->pipes.readBytes(reinterpret_cast<char*>(&code), sizeof(PamIpcExitCode));

			if (!ok) goto fail;

			qCDebug(logPam) << "Subprocess exited with code" << static_cast<int>(code);

			switch (code) {
			case PamIpcExitCode::Success: emit this->completed(PamResult::Success); break;
			case PamIpcExitCode::AuthFailed: emit this->completed(PamResult::Failed); break;
			case PamIpcExitCode::StartFailed: emit this->error(PamError::StartFailed); break;
			case PamIpcExitCode::MaxTries: emit this->completed(PamResult::MaxTries); break;
			case PamIpcExitCode::PamError: emit this->error(PamError::TryAuthFailed); break;
			case PamIpcExitCode::OtherError: emit this->error(PamError::InternalError); break;
			}

			waitpid(this->childPid, nullptr, 0);
			this->childPid = 0;
		} else if (type == PamIpcEvent::Request) {
			PamIpcRequestFlags flags {};

			ok = this->pipes.readBytes(reinterpret_cast<char*>(&flags), sizeof(PamIpcRequestFlags));

			if (!ok) goto fail;

			auto message = this->pipes.readString(&ok);

			if (!ok) goto fail;

			this->message(QString::fromUtf8(message), flags.error, flags.responseRequired, flags.echo);
		} else {
			qCCritical(logPam) << "Unexpected message from subprocess.";
			goto fail;
		}
	}
	return;

fail:
	qCCritical(logPam) << "Failed to read subprocess request.";
	this->internalError();
}
