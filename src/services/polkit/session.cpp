#include "session.hpp"

#include <glib-object.h>
#include <glib.h>
#include <qobject.h>
#include <qtmetamacros.h>

#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
// This causes a problem with variables of the name.
#undef signals
#include <polkitagent/polkitagent.h>
#define signals Q_SIGNALS

namespace qs::service::polkit {

namespace {
void completedCb(PolkitAgentSession* /*session*/, gboolean gainedAuthorization, gpointer userData) {
	auto* self = static_cast<Session*>(userData);
	emit self->completed(gainedAuthorization);
}

void requestCb(
    PolkitAgentSession* /*session*/,
    const char* message,
    gboolean echo,
    gpointer userData
) {
	auto* self = static_cast<Session*>(userData);
	emit self->request(QString::fromUtf8(message), echo);
}

void showErrorCb(PolkitAgentSession* /*session*/, const char* message, gpointer userData) {
	auto* self = static_cast<Session*>(userData);
	emit self->showError(QString::fromUtf8(message));
}

void showInfoCb(PolkitAgentSession* /*session*/, const char* message, gpointer userData) {
	auto* self = static_cast<Session*>(userData);
	emit self->showInfo(QString::fromUtf8(message));
}
} // namespace

Session::Session(PolkitIdentity* identity, const QString& cookie, QObject* parent)
    : QObject(parent) {
	this->session = polkit_agent_session_new(identity, cookie.toUtf8().constData());

	g_signal_connect(G_OBJECT(this->session), "completed", G_CALLBACK(completedCb), this);
	g_signal_connect(G_OBJECT(this->session), "request", G_CALLBACK(requestCb), this);
	g_signal_connect(G_OBJECT(this->session), "show-error", G_CALLBACK(showErrorCb), this);
	g_signal_connect(G_OBJECT(this->session), "show-info", G_CALLBACK(showInfoCb), this);
}

Session::~Session() {
	// Signals do not need to be disconnected explicitly. This happens during
	// destruction of the gobject. Since we own the session object, we can be
	// sure it is being destroyed after the unref.
	g_object_unref(this->session);
}

void Session::initiate() { polkit_agent_session_initiate(this->session); }

void Session::cancel() { polkit_agent_session_cancel(this->session); }

void Session::respond(const QString& response) {
	polkit_agent_session_response(this->session, response.toUtf8().constData());
}

} // namespace qs::service::polkit
