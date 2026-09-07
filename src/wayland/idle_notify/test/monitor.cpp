#include "monitor.hpp"

#include <qtest.h>
#include <qtestcase.h>

#include "../monitor.hpp"
#include "../proto.hpp"

namespace qs::wayland::idle_notify {

constexpr qreal IDLES_ASAP = 0;
constexpr qreal OUTLASTS_TEST = 3600;
constexpr int IDLE_WAIT_MS = 5000;

void TestIdleMonitor::freshMonitorReportsIdle() {
	if (impl::IdleNotificationManager::instance() == nullptr) {
		QSKIP("compositor does not support ext-idle-notify-v1");
	}

	auto monitor = IdleMonitor();
	monitor.bindableTimeout().setValue(IDLES_ASAP);
	monitor.onPostReload();

	QTRY_VERIFY_WITH_TIMEOUT(monitor.bindableIsIdle().value(), IDLE_WAIT_MS);
}

void TestIdleMonitor::liveTimeoutChangeKeepsIdleSubscription() {
	if (impl::IdleNotificationManager::instance() == nullptr) {
		QSKIP("compositor does not support ext-idle-notify-v1");
	}

	auto monitor = IdleMonitor();
	monitor.bindableTimeout().setValue(OUTLASTS_TEST);
	monitor.onPostReload();

	QVERIFY(!monitor.bindableIsIdle().value());

	monitor.bindableTimeout().setValue(IDLES_ASAP);

	QTRY_VERIFY_WITH_TIMEOUT(monitor.bindableIsIdle().value(), IDLE_WAIT_MS);
}

} // namespace qs::wayland::idle_notify

QTEST_MAIN(qs::wayland::idle_notify::TestIdleMonitor);
