#include "monitor.hpp"
#include <algorithm>

#include <qlogging.h>
#include <qscopeguard.h>
#include <qtypes.h>

#include "proto.hpp"

namespace qs::wayland::idle_notify {

IdleMonitor::~IdleMonitor() { delete this->bNotification.value(); }

void IdleMonitor::onPostReload() {
	this->bParams.setBinding([this] {
		return Params {
		    .enabled = this->bEnabled.value(),
		    .timeout = this->bTimeout.value(),
		    .respectInhibitors = this->bRespectInhibitors.value()
		};
	});

	this->bIsIdle.setBinding([this] {
		auto* notification = this->bNotification.value();
		return notification ? notification->bIsIdle.value() : false;
	});
}

void IdleMonitor::updateNotification() {
	auto* notification = this->bNotification.value();
	delete notification;
	notification = nullptr;

	auto guard = qScopeGuard([&] { this->bNotification = notification; });

	auto params = this->bParams.value();

	if (params.enabled) {
		auto* manager = impl::IdleNotificationManager::instance();

		if (!manager) {
			qWarning() << "Cannot create idle monitor as ext-idle-notify-v1 is not supported by the "
			              "current compositor.";
			return;
		}

		auto timeout = static_cast<quint32>(std::max(0, static_cast<int>(params.timeout * 1000)));
		notification = manager->createIdleNotification(timeout, params.respectInhibitors);
	}
}

} // namespace qs::wayland::idle_notify
