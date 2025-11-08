#pragma once

#include <qstring.h>

#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
// This causes a problem with variables of the name.
#undef signals

#include <glib-object.h>
#include <polkitagent/polkitagent.h>

#define signals Q_SIGNALS

#include "gobjectref.hpp"

namespace qs::service::polkit {
class ListenerCb;
//! All state that comes in from PolKit about an authentication request.
struct AuthRequest {
	//! The action ID that this session is for.
	QString actionId;
	//! Message to present to the user.
	QString message;
	//! Icon name according to the FreeDesktop specification. May be empty.
	QString iconName;
	// Details intentionally omitted because nothing seems to use them.
	QString cookie;
	//! List of users/groups that can be used for authentication.
	std::vector<GObjectRef<PolkitIdentity>> identities;

	//! Implementation detail to mark authentication done.
	GTask* task;
	//! Implementation detail for requests cancelled by agent.
	GCancellable* cancellable;
	//! Callback handler ID for the cancellable.
	gulong handlerId;
	//! Callbacks for the listener
	ListenerCb* cb;

	void complete();
	void cancel(const QString& reason);
};

//! Callback interface for PolkitAgent listener events.
class ListenerCb {
public:
	ListenerCb() = default;
	virtual ~ListenerCb() = default;
	Q_DISABLE_COPY_MOVE(ListenerCb);

	//! Called when the agent registration is complete.
	virtual void registerComplete(bool success) = 0;
	//! Called when an authentication request is initiated by PolKit.
	virtual void initiateAuthentication(AuthRequest* request) = 0;
	//! Called when an authentication request is cancelled by PolKit before completion.
	virtual void cancelAuthentication(AuthRequest* request) = 0;
};
} // namespace qs::service::polkit

G_BEGIN_DECLS

// This is GObject code. By using their naming conventions, we clearly mark it
// as such for the rest of the project.
// NOLINTBEGIN(readability-identifier-naming)

#define QS_TYPE_POLKIT_AGENT (qs_polkit_agent_get_type())
G_DECLARE_FINAL_TYPE(QsPolkitAgent, qs_polkit_agent, QS, POLKIT_AGENT, PolkitAgentListener)

QsPolkitAgent* qs_polkit_agent_new(qs::service::polkit::ListenerCb* cb);
void qs_polkit_agent_register(QsPolkitAgent* agent, const char* path);
void qs_polkit_agent_unregister(QsPolkitAgent* agent);

// NOLINTEND(readability-identifier-naming)

G_END_DECLS
