#pragma once

#include <qobject.h>

// _PolkitIdentity and _PolkitAgentSession are considered reserved identifiers,
// but I am specifically forward declaring those reserved names.

// NOLINTBEGIN(bugprone-reserved-identifier)
using PolkitIdentity = struct _PolkitIdentity;
using PolkitAgentSession = struct _PolkitAgentSession;
// NOLINTEND(bugprone-reserved-identifier)

namespace qs::service::polkit {
//! Represents an authentication session for a specific identity.
class Session: public QObject {
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(Session);

public:
	explicit Session(PolkitIdentity* identity, const QString& cookie, QObject* parent = nullptr);
	~Session() override;

	/// Call this after connecting to the relevant signals.
	void initiate();
	/// Call this to abort a running authentication session.
	void cancel();
	/// Provide a response to an input request.
	void respond(const QString& response);

Q_SIGNALS:
	/// Emitted when the session wants to request input from the user.
	///
	/// The message is a prompt to present to the user.
	/// If echo is false, the user's response should not be displayed (e.g. for passwords).
	void request(const QString& message, bool echo);

	/// Emitted when the authentication session completes.
	///
	/// If success is true, authentication was successful.
	/// Otherwise it failed (e.g. wrong password).
	void completed(bool success);

	/// Emitted when an error message should be shown to the user.
	void showError(const QString& message);

	/// Emitted when an informational message should be shown to the user.
	void showInfo(const QString& message);

private:
	PolkitAgentSession* session = nullptr;
};
} // namespace qs::service::polkit
