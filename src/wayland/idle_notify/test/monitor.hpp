#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::wayland::idle_notify {

class TestIdleMonitor: public QObject {
	Q_OBJECT;

private slots:
	static void freshMonitorReportsIdle();
	static void liveTimeoutChangeKeepsIdleSubscription();
};

} // namespace qs::wayland::idle_notify
