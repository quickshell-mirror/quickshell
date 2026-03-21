#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-ext-idle-notify-v1.h>
#include <qwaylandclientextension.h>
#include <wayland-ext-idle-notify-v1-client-protocol.h>

#include "../../core/logcat.hpp"

namespace qs::wayland::idle_notify {
QS_DECLARE_LOGGING_CATEGORY(logIdleNotify);
}

namespace qs::wayland::idle_notify::impl {

class IdleNotification;

class IdleNotificationManager
    : public QWaylandClientExtensionTemplate<IdleNotificationManager>
    , public QtWayland::ext_idle_notifier_v1 {
public:
	explicit IdleNotificationManager();
	IdleNotification* createIdleNotification(quint32 timeout, bool respectInhibitors);

	static IdleNotificationManager* instance();
};

class IdleNotification
    : public QObject
    , public QtWayland::ext_idle_notification_v1 {
	Q_OBJECT;

public:
	explicit IdleNotification(::ext_idle_notification_v1* notification)
	    : QtWayland::ext_idle_notification_v1(notification) {}

	~IdleNotification() override;
	Q_DISABLE_COPY_MOVE(IdleNotification);

	Q_OBJECT_BINDABLE_PROPERTY(IdleNotification, bool, bIsIdle);

protected:
	void ext_idle_notification_v1_idled() override;
	void ext_idle_notification_v1_resumed() override;
};

} // namespace qs::wayland::idle_notify::impl
