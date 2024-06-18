#include "conversation.hpp"
#include <utility>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmutex.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <security/_pam_types.h>
#include <security/pam_appl.h>

Q_LOGGING_CATEGORY(logPam, "quickshell.service.pam", QtWarningMsg);

QString PamError::toString(PamError::Enum value) {
	switch (value) {
	case ConnectionFailed: return "Failed to connect to pam";
	case TryAuthFailed: return "Failed to try authenticating";
	default: return "Invalid error";
	}
}

QString PamResult::toString(PamResult::Enum value) {
	switch (value) {
	case Success: return "Success";
	case Failed: return "Failed";
	case Error: return "Error occurred while authenticating";
	case MaxTries: return "The authentication method has no more attempts available";
	// case Expired: return "The account has expired";
	// case PermissionDenied: return "Permission denied";
	default: return "Invalid result";
	}
}

void PamConversation::run() {
	auto conv = pam_conv {
	    .conv = &PamConversation::conversation,
	    .appdata_ptr = this,
	};

	pam_handle_t* handle = nullptr;

	qCInfo(logPam) << this << "Starting pam session for user" << this->user << "with config"
	               << this->config << "in configdir" << this->configDir;

	auto result = pam_start_confdir(
	    this->config.toStdString().c_str(),
	    this->user.toStdString().c_str(),
	    &conv,
	    this->configDir.toStdString().c_str(),
	    &handle
	);

	if (result != PAM_SUCCESS) {
		qCCritical(logPam) << this << "Unable to start pam conversation with error"
		                   << QString(pam_strerror(handle, result));
		emit this->error(PamError::ConnectionFailed);
		this->deleteLater();
		return;
	}

	result = pam_authenticate(handle, 0);

	// Seems to require root and quickshell should not run as root.
	// if (result == PAM_SUCCESS) {
	//   result = pam_acct_mgmt(handle, 0);
	// }

	switch (result) {
	case PAM_SUCCESS:
		qCInfo(logPam) << this << "ended with successful authentication.";
		emit this->completed(PamResult::Success);
		break;
	case PAM_AUTH_ERR:
		qCInfo(logPam) << this << "ended with failed authentication.";
		emit this->completed(PamResult::Failed);
		break;
	case PAM_MAXTRIES:
		qCInfo(logPam) << this << "ended with failure: max tries.";
		emit this->completed(PamResult::MaxTries);
		break;
	/*case PAM_ACCT_EXPIRED:
		qCInfo(logPam) << this << "ended with failure: account expiration.";
		emit this->completed(PamResult::Expired);
		break;
	case PAM_PERM_DENIED:
		qCInfo(logPam) << this << "ended with failure: permission denied.";
		emit this->completed(PamResult::PermissionDenied);
		break;*/
	default:
		qCCritical(logPam) << this << "ended with error:" << QString(pam_strerror(handle, result));
		emit this->error(PamError::TryAuthFailed);
		break;
	}

	result = pam_end(handle, result);
	if (result != PAM_SUCCESS) {
		qCCritical(logPam) << this << "Failed to end pam conversation with error code"
		                   << QString(pam_strerror(handle, result));
	}

	this->deleteLater();
}

void PamConversation::abort() {
	qCDebug(logPam) << "Abort requested for" << this;
	auto locker = QMutexLocker(&this->wakeMutex);
	this->mAbort = true;
	this->waker.wakeOne();
}

void PamConversation::respond(QString response) {
	qCDebug(logPam) << "Set response for" << this;
	auto locker = QMutexLocker(&this->wakeMutex);
	this->response = std::move(response);
	this->hasResponse = true;
	this->waker.wakeOne();
}

int PamConversation::conversation(
    int msgCount,
    const pam_message** msgArray,
    pam_response** responseArray,
    void* appdata
) {
	auto* delegate = static_cast<PamConversation*>(appdata);

	{
		auto locker = QMutexLocker(&delegate->wakeMutex);
		if (delegate->mAbort) {
			return PAM_ERROR_MSG;
		}
	}

	// freed by libc so must be alloc'd by it.
	auto* responses = static_cast<pam_response*>(calloc(msgCount, sizeof(pam_response))); // NOLINT

	for (auto i = 0; i < msgCount; i++) {
		const auto* message = msgArray[i]; // NOLINT
		auto& response = responses[i];     // NOLINT

		auto msgString = QString(message->msg);
		auto messageChanged = true; // message->msg_style != PAM_PROMPT_ECHO_OFF;
		auto isError = message->msg_style == PAM_ERROR_MSG;
		auto responseRequired =
		    message->msg_style == PAM_PROMPT_ECHO_OFF || message->msg_style == PAM_PROMPT_ECHO_ON;

		qCDebug(logPam) << delegate << "got new message message:" << msgString
		                << "messageChanged:" << messageChanged << "isError:" << isError
		                << "responseRequired" << responseRequired;

		delegate->hasResponse = false;
		emit delegate->message(msgString, messageChanged, isError, responseRequired);

		{
			auto locker = QMutexLocker(&delegate->wakeMutex);

			if (delegate->mAbort) {
				free(responses); // NOLINT
				return PAM_ERROR_MSG;
			}

			if (responseRequired) {
				if (!delegate->hasResponse) {
					delegate->waker.wait(locker.mutex());

					if (delegate->mAbort) {
						free(responses); // NOLINT
						return PAM_ERROR_MSG;
					}
				}

				if (!delegate->hasResponse) {
					qCCritical(logPam
					) << "Pam conversation requires response and does not have one. This should not happen.";
				}

				response.resp = strdup(delegate->response.toStdString().c_str()); // NOLINT (include error)
			}
		}
	}

	*responseArray = responses;
	return PAM_SUCCESS;
}
