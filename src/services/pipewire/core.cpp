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
#include <spa/utils/defs.h>
#include <spa/utils/hook.h>

namespace qs::service::pipewire {

namespace {
Q_LOGGING_CATEGORY(logLoop, "quickshell.service.pipewire.loop", QtWarningMsg);
}

PwCore::PwCore(QObject* parent): QObject(parent), notifier(QSocketNotifier::Read) {
	qCInfo(logLoop) << "Creating pipewire event loop.";
	pw_init(nullptr, nullptr);

	this->loop = pw_loop_new(nullptr);
	if (this->loop == nullptr) {
		qCCritical(logLoop) << "Failed to create pipewire event loop.";
		return;
	}

	this->context = pw_context_new(this->loop, nullptr, 0);
	if (this->context == nullptr) {
		qCCritical(logLoop) << "Failed to create pipewire context.";
		return;
	}

	qCInfo(logLoop) << "Connecting to pipewire server.";
	this->core = pw_context_connect(this->context, nullptr, 0);
	if (this->core == nullptr) {
		qCCritical(logLoop) << "Failed to connect pipewire context. Errno:" << errno;
		return;
	}

	qCInfo(logLoop) << "Linking pipewire event loop.";
	// Tie the pw event loop into qt.
	auto fd = pw_loop_get_fd(this->loop);
	this->notifier.setSocket(fd);
	QObject::connect(&this->notifier, &QSocketNotifier::activated, this, &PwCore::poll);
	this->notifier.setEnabled(true);
}

PwCore::~PwCore() {
	qCInfo(logLoop) << "Destroying PwCore.";

	if (this->loop != nullptr) {
		if (this->context != nullptr) {
			if (this->core != nullptr) {
				pw_core_disconnect(this->core);
			}

			pw_context_destroy(this->context);
		}

		pw_loop_destroy(this->loop);
	}
}

bool PwCore::isValid() const {
	// others must init first
	return this->core != nullptr;
}

void PwCore::poll() {
	qCDebug(logLoop) << "Pipewire event loop received new events, iterating.";
	// Spin pw event loop.
	pw_loop_iterate(this->loop, 0);
	qCDebug(logLoop) << "Done iterating pipewire event loop.";
	emit this->polled();
}

SpaHook::SpaHook() { // NOLINT
	spa_zero(this->hook);
}

void SpaHook::remove() {
	spa_hook_remove(&this->hook);
	spa_zero(this->hook);
}

} // namespace qs::service::pipewire
