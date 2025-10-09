#include "listener.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <polkit/polkit.h>
#include <polkit/polkittypes.h>
#include <polkitagent/polkitagent.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <unistd.h>

#include "../../core/logcat.hpp"
#include "gobjectref.hpp"
#include "qml.hpp"

namespace {
QS_LOGGING_CATEGORY(logPolkitListener, "quickshell.service.polkit.listener", QtWarningMsg);
}

using qs::service::polkit::GObjectRef;

// This is mostly GObject code, we follow their naming conventions for improved
// clarity and to mark it as such. Additionally, many methods need to be static
// to conform with the expected declarations.
// NOLINTBEGIN(readability-identifier-naming,misc-use-anonymous-namespace)

using QsPolkitAgent = struct _QsPolkitAgent {
	PolkitAgentListener parent_instance;

	qs::service::polkit::ListenerCb* cb;
	gpointer registration_handle;
};

G_DEFINE_TYPE(QsPolkitAgent, qs_polkit_agent, POLKIT_AGENT_TYPE_LISTENER)

static void initiate_authentication(
    PolkitAgentListener* listener,
    const gchar* actionId,
    const gchar* message,
    const gchar* iconName,
    PolkitDetails* details,
    const gchar* cookie,
    GList* identities,
    GCancellable* cancellable,
    GAsyncReadyCallback callback,
    gpointer userData
);

static gboolean
initiate_authentication_finish(PolkitAgentListener* listener, GAsyncResult* result, GError** error);

static void qs_polkit_agent_init(QsPolkitAgent* self) {
	self->cb = nullptr;
	self->registration_handle = nullptr;
}

static void qs_polkit_agent_finalize(GObject* object) {
	if (G_OBJECT_CLASS(qs_polkit_agent_parent_class))
		G_OBJECT_CLASS(qs_polkit_agent_parent_class)->finalize(object);
}

static void qs_polkit_agent_class_init(QsPolkitAgentClass* klass) {
	GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = qs_polkit_agent_finalize;

	PolkitAgentListenerClass* listener_class = POLKIT_AGENT_LISTENER_CLASS(klass);
	listener_class->initiate_authentication = initiate_authentication;
	listener_class->initiate_authentication_finish = initiate_authentication_finish;
}

QsPolkitAgent* qs_polkit_agent_new(qs::service::polkit::ListenerCb* cb) {
	QsPolkitAgent* self = QS_POLKIT_AGENT(g_object_new(QS_TYPE_POLKIT_AGENT, nullptr));
	self->cb = cb;
	return self;
}

struct RegisterCbData {
	GObjectRef<QsPolkitAgent> agent;
	std::string path;
};

static void qs_polkit_agent_register_cb(GObject* /*unused*/, GAsyncResult* res, gpointer userData);
void qs_polkit_agent_register(QsPolkitAgent* agent, const char* path) {
	if (path == nullptr || *path == '\0') {
		qCWarning(logPolkitListener) << "cannot register listener without a path set.";
		agent->cb->registerComplete(false);
		return;
	}

	auto* data = new RegisterCbData {.agent = GObjectRef(agent), .path = path};
	polkit_unix_session_new_for_process(getpid(), nullptr, &qs_polkit_agent_register_cb, data);
}

static void qs_polkit_agent_register_cb(GObject* /*unused*/, GAsyncResult* res, gpointer userData) {
	std::unique_ptr<RegisterCbData> data(reinterpret_cast<RegisterCbData*>(userData));

	GError* error = nullptr;
	auto* subject = polkit_unix_session_new_for_process_finish(res, &error);

	if (subject == nullptr || error != nullptr) {
		qCWarning(logPolkitListener) << "failed to create subject for listener:"
		                             << (error ? error->message : "<unknown error>");
		g_clear_error(&error);
		data->agent->cb->registerComplete(false);
		return;
	}

	data->agent->registration_handle = polkit_agent_listener_register(
	    POLKIT_AGENT_LISTENER(data->agent.get()),
	    POLKIT_AGENT_REGISTER_FLAGS_NONE,
	    subject,
	    data->path.c_str(),
	    nullptr,
	    &error
	);

	g_object_unref(subject);

	if (error != nullptr) {
		qCWarning(logPolkitListener) << "failed to register listener:" << error->message;
		g_clear_error(&error);
		data->agent->cb->registerComplete(false);
		return;
	}

	data->agent->cb->registerComplete(true);
}

void qs_polkit_agent_unregister(QsPolkitAgent* agent) {
	if (agent->registration_handle != nullptr) {
		polkit_agent_listener_unregister(agent->registration_handle);
		agent->registration_handle = nullptr;
	}
}

static void authentication_cancelled_cb(GCancellable* /*unused*/, gpointer userData) {
	auto* request = static_cast<qs::service::polkit::AuthRequest*>(userData);
	request->cb->cancelAuthentication(request);
}

static void initiate_authentication(
    PolkitAgentListener* listener,
    const gchar* actionId,
    const gchar* message,
    const gchar* iconName,
    PolkitDetails* /*unused*/,
    const gchar* cookie,
    GList* identities,
    GCancellable* cancellable,
    GAsyncReadyCallback callback,
    gpointer userData
) {
	auto* self = QS_POLKIT_AGENT(listener);

	auto* asyncResult = g_task_new(reinterpret_cast<GObject*>(self), nullptr, callback, userData);

	// Identities may be duplicated, so we use the hash to filter them out.
	std::unordered_set<guint> identitySet;
	std::vector<GObjectRef<PolkitIdentity>> identityVector;
	for (auto* item = g_list_first(identities); item != nullptr; item = g_list_next(item)) {
		auto* identity = static_cast<PolkitIdentity*>(item->data);
		if (identitySet.contains(polkit_identity_hash(identity))) continue;

		identitySet.insert(polkit_identity_hash(identity));
		// The caller unrefs all identities after we return, therefore we need to
		// take our own reference for the identities we keep. Our wrapper does
		// this automatically.
		identityVector.emplace_back(identity);
	}

	// The original strings are freed by the caller after we return, so we
	// copy them into QStrings.
	auto* request = new qs::service::polkit::AuthRequest {
	    .actionId = QString::fromUtf8(actionId),
	    .message = QString::fromUtf8(message),
	    .iconName = QString::fromUtf8(iconName),
	    .cookie = QString::fromUtf8(cookie),
	    .identities = std::move(identityVector),

	    .task = asyncResult,
	    .cancellable = cancellable,
	    .handlerId = 0,
	    .cb = self->cb
	};

	if (cancellable != nullptr) {
		request->handlerId = g_cancellable_connect(
		    cancellable,
		    reinterpret_cast<GCallback>(authentication_cancelled_cb),
		    request,
		    nullptr
		);
	}

	self->cb->initiateAuthentication(request);
}

static gboolean initiate_authentication_finish(
    PolkitAgentListener* /*unused*/,
    GAsyncResult* result,
    GError** error
) {
	return g_task_propagate_boolean(G_TASK(result), error);
}

namespace qs::service::polkit {
// While these functions can be const since they do not modify member variables,
// they are logically non-const since they modify the state of the
// authentication request. Therefore, we do not mark them as const.
// NOLINTBEGIN(readability-make-member-function-const)
void AuthRequest::complete() { g_task_return_boolean(this->task, true); }

void AuthRequest::cancel(const QString& reason) {
	auto utf8Reason = reason.toUtf8();
	g_task_return_new_error(
	    this->task,
	    POLKIT_ERROR,
	    POLKIT_ERROR_CANCELLED,
	    "%s",
	    utf8Reason.constData()
	);
}
// NOLINTEND(readability-make-member-function-const)
} // namespace qs::service::polkit

// NOLINTEND(readability-identifier-naming,misc-use-anonymous-namespace)