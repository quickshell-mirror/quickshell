#pragma once

#include <deque>

#include <qobject.h>
#include <qproperty.h>

#include "flow.hpp"
#include "gobjectref.hpp"
#include "listener.hpp"

namespace qs::service::polkit {
class PolkitAgent;

class PolkitAgentImpl
    : public QObject
    , public ListenerCb {
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(PolkitAgentImpl);

public:
	~PolkitAgentImpl() override;

	static PolkitAgentImpl* tryGetOrCreate(PolkitAgent* agent);
	static PolkitAgentImpl* tryGet(const PolkitAgent* agent);
	static PolkitAgentImpl* tryTakeoverOrCreate(PolkitAgent* agent);
	static void onEndOfQmlAgent(PolkitAgent* agent);

	[[nodiscard]] QBindable<AuthFlow*> activeFlow() { return &this->bActiveFlow; };
	[[nodiscard]] QBindable<bool> isRegistered() { return &this->bIsRegistered; };

	[[nodiscard]] const QString& getPath() const { return this->path; }
	void setPath(const QString& path);

	void initiateAuthentication(AuthRequest* request) override;
	void cancelAuthentication(AuthRequest* request) override;
	void registerComplete(bool success) override;

	void cancelAllRequests(const QString& reason);

signals:
	void activeFlowChanged();
	void isRegisteredChanged();

private:
	PolkitAgentImpl(PolkitAgent* agent);

	static PolkitAgentImpl* instance;

	/// Start handling of the next authentication request in the queue.
	void activateAuthenticationRequest();
	/// Finalize and remove the current authentication request.
	void finishAuthenticationRequest();

	GObjectRef<QsPolkitAgent> listener;
	PolkitAgent* qmlAgent = nullptr;
	QString path;

	std::deque<AuthRequest*> queuedRequests;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(PolkitAgentImpl, AuthFlow*, bActiveFlow, &PolkitAgentImpl::activeFlowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(PolkitAgentImpl, bool, bIsRegistered, &PolkitAgentImpl::isRegisteredChanged);
	// clang-format on
};
} // namespace qs::service::polkit
