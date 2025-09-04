#include "proto.hpp"

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandintegration_p.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtypes.h>
#include <qwayland-ext-idle-notify-v1.h>
#include <qwaylandclientextension.h>
#include <wayland-ext-idle-notify-v1-client-protocol.h>

#include "../../core/logcat.hpp"

namespace qs::wayland::idle_notify {
QS_LOGGING_CATEGORY(logIdleNotify, "quickshell.wayland.idle_notify", QtWarningMsg);
}

namespace qs::wayland::idle_notify::impl {

IdleNotificationManager::IdleNotificationManager(): QWaylandClientExtensionTemplate(2) {
	this->initialize();
}

IdleNotificationManager* IdleNotificationManager::instance() {
	static auto* instance = new IdleNotificationManager(); // NOLINT
	return instance->isInitialized() ? instance : nullptr;
}

IdleNotification*
IdleNotificationManager::createIdleNotification(quint32 timeout, bool respectInhibitors) {
	if (!respectInhibitors
	    && this->QtWayland::ext_idle_notifier_v1::version()
	           < EXT_IDLE_NOTIFIER_V1_GET_INPUT_IDLE_NOTIFICATION_SINCE_VERSION)
	{
		qCWarning(logIdleNotify) << "Cannot ignore inhibitors for new idle notifier: Compositor does "
		                            "not support protocol version 2.";

		respectInhibitors = true;
	}

	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();
	auto* inputDevice = display->lastInputDevice();
	if (inputDevice == nullptr) inputDevice = display->defaultInputDevice();
	if (inputDevice == nullptr) {
		qCCritical(logIdleNotify) << "Could not create idle notifier: No seat.";
		return nullptr;
	}

	::ext_idle_notification_v1* notification = nullptr;
	if (respectInhibitors) notification = this->get_idle_notification(timeout, inputDevice->object());
	else notification = this->get_input_idle_notification(timeout, inputDevice->object());

	auto* wrapper = new IdleNotification(notification);
	qCDebug(logIdleNotify) << "Created" << wrapper << "with timeout:" << timeout
	                       << "respects inhibitors:" << respectInhibitors;
	return wrapper;
}

IdleNotification::~IdleNotification() {
	qCDebug(logIdleNotify) << "Destroyed" << this;
	if (this->isInitialized()) this->destroy();
}

void IdleNotification::ext_idle_notification_v1_idled() {
	qCDebug(logIdleNotify) << this << "has been marked idle";
	this->bIsIdle = true;
}

void IdleNotification::ext_idle_notification_v1_resumed() {
	qCDebug(logIdleNotify) << this << "has been marked resumed";
	this->bIsIdle = false;
}

} // namespace qs::wayland::idle_notify::impl
