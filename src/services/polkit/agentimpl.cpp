#include "agentimpl.hpp"
#include <algorithm>
#include <utility>

#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>

#include "../../core/generation.hpp"
#include "../../core/logcat.hpp"
#include "gobjectref.hpp"
#include "listener.hpp"
#include "qml.hpp"

namespace {
QS_LOGGING_CATEGORY(logPolkit, "quickshell.service.polkit", QtWarningMsg);
}

namespace qs::service::polkit {
PolkitAgentImpl* PolkitAgentImpl::instance = nullptr;

PolkitAgentImpl::PolkitAgentImpl(PolkitAgent* agent)
    : QObject(nullptr)
    , listener(qs_polkit_agent_new(this), G_OBJECT_NO_REF)
    , qmlAgent(agent)
    , path(this->qmlAgent->path()) {
	auto utf8Path = this->path.toUtf8();
	qs_polkit_agent_register(this->listener.get(), utf8Path.constData());
}

PolkitAgentImpl::~PolkitAgentImpl() { this->cancelAllRequests("PolkitAgent is being destroyed"); }

void PolkitAgentImpl::cancelAllRequests(const QString& reason) {
	for (; !this->queuedRequests.empty(); this->queuedRequests.pop_back()) {
		AuthRequest* req = this->queuedRequests.back();
		qCDebug(logPolkit) << "destroying queued authentication request for action" << req->actionId;
		req->cancel(reason);
		delete req;
	}

	auto* flow = this->bActiveFlow.value();
	if (flow) {
		flow->cancelAuthenticationRequest();
		flow->deleteLater();
	}

	if (this->bIsRegistered.value()) qs_polkit_agent_unregister(this->listener.get());
}

PolkitAgentImpl* PolkitAgentImpl::tryGetOrCreate(PolkitAgent* agent) {
	if (instance == nullptr) instance = new PolkitAgentImpl(agent);
	if (instance->qmlAgent == agent) return instance;
	return nullptr;
}

PolkitAgentImpl* PolkitAgentImpl::tryGet(const PolkitAgent* agent) {
	if (instance == nullptr) return nullptr;
	if (instance->qmlAgent == agent) return instance;
	return nullptr;
}

PolkitAgentImpl* PolkitAgentImpl::tryTakeoverOrCreate(PolkitAgent* agent) {
	if (auto* impl = tryGetOrCreate(agent); impl != nullptr) return impl;

	auto* prevGen = EngineGeneration::findObjectGeneration(instance->qmlAgent);
	auto* myGen = EngineGeneration::findObjectGeneration(agent);
	if (prevGen == myGen) return nullptr;

	qCDebug(logPolkit) << "taking over listener from previous generation";
	instance->qmlAgent = agent;
	instance->setPath(agent->path());

	return instance;
}

void PolkitAgentImpl::onEndOfQmlAgent(PolkitAgent* agent) {
	if (instance != nullptr && instance->qmlAgent == agent) {
		delete instance;
		instance = nullptr;
	}
}

void PolkitAgentImpl::setPath(const QString& path) {
	if (this->path == path) return;

	this->path = path;
	auto utf8Path = path.toUtf8();

	this->cancelAllRequests("PolkitAgent path changed");
	qs_polkit_agent_unregister(this->listener.get());
	this->bIsRegistered = false;

	qs_polkit_agent_register(this->listener.get(), utf8Path.constData());
}

void PolkitAgentImpl::registerComplete(bool success) {
	if (success) this->bIsRegistered = true;
	else qCWarning(logPolkit) << "failed to register listener on path" << this->qmlAgent->path();
}

void PolkitAgentImpl::initiateAuthentication(AuthRequest* request) {
	qCDebug(logPolkit) << "incoming authentication request for action" << request->actionId;

	this->queuedRequests.emplace_back(request);

	if (this->queuedRequests.size() == 1) {
		this->activateAuthenticationRequest();
	}
}

void PolkitAgentImpl::cancelAuthentication(AuthRequest* request) {
	qCDebug(logPolkit) << "cancelling authentication request from agent";

	auto* flow = this->bActiveFlow.value();
	if (flow && flow->authRequest() == request) {
		flow->cancelFromAgent();
	} else if (auto it = std::ranges::find(this->queuedRequests, request);
	           it != this->queuedRequests.end())
	{
		qCDebug(logPolkit) << "removing queued authentication request for action" << (*it)->actionId;
		(*it)->cancel("Authentication request was cancelled");
		delete (*it);
		this->queuedRequests.erase(it);
	} else {
		qCWarning(logPolkit) << "the cancelled request was not found in the queue.";
	}
}

void PolkitAgentImpl::activateAuthenticationRequest() {
	if (this->queuedRequests.empty()) return;

	AuthRequest* req = this->queuedRequests.front();
	this->queuedRequests.pop_front();
	qCDebug(logPolkit) << "activating authentication request for action" << req->actionId
	                   << ", cookie: " << req->cookie;

	QList<Identity*> identities;
	for (auto& identity: req->identities) {
		auto* obj = Identity::fromPolkitIdentity(identity);
		if (obj) identities.append(obj);
	}
	if (identities.isEmpty()) {
		qCWarning(logPolkit
		) << "no supported identities available for authentication request, cancelling.";
		req->cancel("Error requesting authentication: no supported identities available.");
		delete req;
		return;
	}

	this->bActiveFlow = new AuthFlow(req, std::move(identities));

	QObject::connect(
	    this->bActiveFlow.value(),
	    &AuthFlow::isCompletedChanged,
	    this,
	    &PolkitAgentImpl::finishAuthenticationRequest
	);

	emit this->qmlAgent->authenticationRequestStarted();
}

void PolkitAgentImpl::finishAuthenticationRequest() {
	if (!this->bActiveFlow.value()) return;

	qCDebug(logPolkit) << "finishing authentication request for action"
	                   << this->bActiveFlow.value()->actionId();

	this->bActiveFlow.value()->deleteLater();

	if (!this->queuedRequests.empty()) {
		this->activateAuthenticationRequest();
	} else {
		this->bActiveFlow = nullptr;
	}
}
} // namespace qs::service::polkit
