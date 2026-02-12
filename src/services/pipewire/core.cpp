#include "core.hpp"
#include <cerrno>

#include <pipewire/context.h>
#include <pipewire/core.h>
#include <pipewire/loop.h>
#include <pipewire/pipewire.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/utils/defs.h>
#include <spa/utils/hook.h>

#include "../../core/logcat.hpp"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logLoop, "quickshell.service.pipewire.loop", QtWarningMsg);
}

const pw_core_events PwCore::EVENTS = {
    .version = PW_VERSION_CORE_EVENTS,
    .info = nullptr,
    .done = &PwCore::onSync,
    .ping = nullptr,
    .error = &PwCore::onError,
    .remove_id = nullptr,
    .bound_id = nullptr,
    .add_mem = nullptr,
    .remove_mem = nullptr,
    .bound_props = nullptr,
};

PwCore::PwCore(QObject* parent): QObject(parent), notifier(QSocketNotifier::Read) {
	pw_init(nullptr, nullptr);
}

bool PwCore::start(bool retry) {
	if (this->core != nullptr) return true;

	qCInfo(logLoop) << "Creating pipewire event loop.";

	this->loop = pw_loop_new(nullptr);
	if (this->loop == nullptr) {
		if (retry) {
			qCInfo(logLoop) << "Failed to create pipewire event loop.";
		} else {
			qCCritical(logLoop) << "Failed to create pipewire event loop.";
		}
		this->shutdown();
		return false;
	}

	this->context = pw_context_new(this->loop, nullptr, 0);
	if (this->context == nullptr) {
		if (retry) {
			qCInfo(logLoop) << "Failed to create pipewire context.";
		} else {
			qCCritical(logLoop) << "Failed to create pipewire context.";
		}
		this->shutdown();
		return false;
	}

	qCInfo(logLoop) << "Connecting to pipewire server.";
	this->core = pw_context_connect(this->context, nullptr, 0);
	if (this->core == nullptr) {
		if (retry) {
			qCInfo(logLoop) << "Failed to connect pipewire context. Errno:" << errno;
		} else {
			qCCritical(logLoop) << "Failed to connect pipewire context. Errno:" << errno;
		}
		this->shutdown();
		return false;
	}

	pw_core_add_listener(this->core, &this->listener.hook, &PwCore::EVENTS, this);

	qCInfo(logLoop) << "Linking pipewire event loop.";
	// Tie the pw event loop into qt.
	auto fd = pw_loop_get_fd(this->loop);
	this->notifier.setSocket(fd);
	QObject::connect(&this->notifier, &QSocketNotifier::activated, this, &PwCore::poll);
	this->notifier.setEnabled(true);

	return true;
}

void PwCore::shutdown() {
	if (this->core != nullptr) {
		this->listener.remove();
		pw_core_disconnect(this->core);
		this->core = nullptr;
	}

	if (this->context != nullptr) {
		pw_context_destroy(this->context);
		this->context = nullptr;
	}

	if (this->loop != nullptr) {
		pw_loop_destroy(this->loop);
		this->loop = nullptr;
	}

	this->notifier.setEnabled(false);
	QObject::disconnect(&this->notifier, nullptr, this, nullptr);
}

PwCore::~PwCore() {
	qCInfo(logLoop) << "Destroying PwCore.";
	this->shutdown();
}

bool PwCore::isValid() const {
	// others must init first
	return this->core != nullptr;
}

void PwCore::poll() {
	if (this->loop == nullptr) return;
	qCDebug(logLoop) << "Pipewire event loop received new events, iterating.";
	// Spin pw event loop.
	pw_loop_iterate(this->loop, 0);
	qCDebug(logLoop) << "Done iterating pipewire event loop.";
	emit this->polled();
}

qint32 PwCore::sync(quint32 id) const {
	// Seq param doesn't seem to do anything. Seq is instead the returned value.
	return pw_core_sync(this->core, id, 0);
}

void PwCore::onSync(void* data, quint32 id, qint32 seq) {
	auto* self = static_cast<PwCore*>(data);
	emit self->synced(id, seq);
}

void PwCore::onError(void* data, quint32 id, qint32 /*seq*/, qint32 res, const char* message) {
	auto* self = static_cast<PwCore*>(data);

	if (message != nullptr) {
		qCWarning(logLoop) << "Fatal pipewire error on object" << id << "with code" << res << message;
	} else {
		qCWarning(logLoop) << "Fatal pipewire error on object" << id << "with code" << res;
	}

	emit self->fatalError();
}

SpaHook::SpaHook() { // NOLINT
	spa_zero(this->hook);
}

void SpaHook::remove() {
	spa_hook_remove(&this->hook);
	spa_zero(this->hook);
}

} // namespace qs::service::pipewire
