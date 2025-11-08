#include "flow.hpp"
#include <utility>

#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlinfo.h>
#include <qtmetamacros.h>

#include "../../core/logcat.hpp"
#include "identity.hpp"
#include "qml.hpp"
#include "session.hpp"

namespace {
QS_LOGGING_CATEGORY(logPolkitState, "quickshell.service.polkit.state", QtWarningMsg);
}

namespace qs::service::polkit {
AuthFlow::AuthFlow(AuthRequest* request, QList<Identity*>&& identities, QObject* parent)
    : QObject(parent)
    , mRequest(request)
    , mIdentities(std::move(identities))
    , bSelectedIdentity(this->mIdentities.isEmpty() ? nullptr : this->mIdentities.first()) {
	// We reject auth requests with no identities before a flow is created.
	// This should never happen.
	if (!this->bSelectedIdentity.value())
		qCFatal(logPolkitState) << "AuthFlow created with no valid identities!";

	for (auto* identity: this->mIdentities) {
		identity->setParent(this);
	}

	this->setupSession();
}

AuthFlow::~AuthFlow() { delete this->mRequest; };

void AuthFlow::setSelectedIdentity(Identity* identity) {
	if (this->bSelectedIdentity.value() == identity) return;
	if (!identity) {
		qmlWarning(this) << "Cannot set selected identity to null.";
		return;
	}
	this->bSelectedIdentity = identity;
	this->currentSession->cancel();
	this->setupSession();
}

void AuthFlow::cancelFromAgent() {
	if (!this->currentSession) return;

	qCDebug(logPolkitState) << "cancelling authentication request from agent";

	// Session cancel can immediately call the cancel handler, which also
	// performs property updates.
	Qt::beginPropertyUpdateGroup();
	this->bIsCancelled = true;
	this->currentSession->cancel();
	Qt::endPropertyUpdateGroup();

	emit this->authenticationRequestCancelled();

	this->mRequest->cancel("Authentication request cancelled by agent.");
}

void AuthFlow::submit(const QString& value) {
	if (!this->currentSession) return;

	qCDebug(logPolkitState) << "submitting response to authentication request";

	this->currentSession->respond(value);

	Qt::beginPropertyUpdateGroup();
	this->bIsResponseRequired = false;
	this->bInputPrompt = QString();
	this->bResponseVisible = false;
	Qt::endPropertyUpdateGroup();
}

void AuthFlow::cancelAuthenticationRequest() {
	if (!this->currentSession) return;

	qCDebug(logPolkitState) << "cancelling authentication request by user request";

	// Session cancel can immediately call the cancel handler, which also
	// performs property updates.
	Qt::beginPropertyUpdateGroup();
	this->bIsCancelled = true;
	this->currentSession->cancel();
	Qt::endPropertyUpdateGroup();

	this->mRequest->cancel("Authentication request cancelled by user.");
}

void AuthFlow::setupSession() {
	delete this->currentSession;

	qCDebug(logPolkitState) << "setting up session for identity"
	                        << this->bSelectedIdentity.value()->name();

	this->currentSession = new Session(
	    this->bSelectedIdentity.value()->polkitIdentity.get(),
	    this->mRequest->cookie,
	    this
	);
	QObject::connect(this->currentSession, &Session::request, this, &AuthFlow::request);
	QObject::connect(this->currentSession, &Session::completed, this, &AuthFlow::completed);
	QObject::connect(this->currentSession, &Session::showError, this, &AuthFlow::showError);
	QObject::connect(this->currentSession, &Session::showInfo, this, &AuthFlow::showInfo);
	this->currentSession->initiate();
}

void AuthFlow::request(const QString& message, bool echo) {
	Qt::beginPropertyUpdateGroup();
	this->bIsResponseRequired = true;
	this->bInputPrompt = message;
	this->bResponseVisible = echo;
	Qt::endPropertyUpdateGroup();
}

void AuthFlow::completed(bool gainedAuthorization) {
	qCDebug(logPolkitState) << "authentication session completed, gainedAuthorization ="
	                        << gainedAuthorization << ", isCancelled =" << this->bIsCancelled.value();

	if (gainedAuthorization) {
		Qt::beginPropertyUpdateGroup();
		this->bIsCompleted = true;
		this->bIsSuccessful = true;
		Qt::endPropertyUpdateGroup();

		this->mRequest->complete();

		emit this->authenticationSucceeded();
	} else if (this->bIsCancelled.value()) {
		Qt::beginPropertyUpdateGroup();
		this->bIsCompleted = true;
		this->bIsSuccessful = false;
		Qt::endPropertyUpdateGroup();
	} else {
		this->bFailed = true;
		emit this->authenticationFailed();

		this->setupSession();
	}
}

void AuthFlow::showError(const QString& message) {
	Qt::beginPropertyUpdateGroup();
	this->bSupplementaryMessage = message;
	this->bSupplementaryIsError = true;
	Qt::endPropertyUpdateGroup();
}

void AuthFlow::showInfo(const QString& message) {
	Qt::beginPropertyUpdateGroup();
	this->bSupplementaryMessage = message;
	this->bSupplementaryIsError = false;
	Qt::endPropertyUpdateGroup();
}
} // namespace qs::service::polkit
