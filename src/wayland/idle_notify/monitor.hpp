#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/reload.hpp"
#include "proto.hpp"

namespace qs::wayland::idle_notify {

///! Provides a notification when a wayland session is makred idle
/// An idle monitor detects when the user stops providing input for a period of time.
///
/// > [!NOTE] Using an idle monitor requires the compositor support the [ext-idle-notify-v1] protocol.
///
/// [ext-idle-notify-v1]: https://wayland.app/protocols/ext-idle-notify-v1
class IdleMonitor: public PostReloadHook {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// If the idle monitor should be enabled. Defaults to true.
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged);
	/// The amount of time in seconds the idle monitor should wait before reporting an idle state.
	///
	/// Defaults to zero, which reports idle status immediately.
	Q_PROPERTY(qreal timeout READ default WRITE default NOTIFY timeoutChanged BINDABLE bindableTimeout);
	/// When set to true, @@isIdle will depend on both user interaction and active idle inhibitors.
	/// When false, the value will depend solely on user interaction. Defaults to true.
	Q_PROPERTY(bool respectInhibitors READ default WRITE default NOTIFY respectInhibitorsChanged BINDABLE bindableRespectInhibitors);
	/// This property is true if the user has been idle for at least @@timeout.
	/// What is considered to be idle is influenced by @@respectInhibitors.
	Q_PROPERTY(bool isIdle READ default NOTIFY isIdleChanged BINDABLE bindableIsIdle);
	// clang-format on

public:
	IdleMonitor() = default;
	~IdleMonitor() override;
	Q_DISABLE_COPY_MOVE(IdleMonitor);

	void onPostReload() override;

	[[nodiscard]] bool isEnabled() const { return this->bNotification.value(); }
	void setEnabled(bool enabled) { this->bEnabled = enabled; }

	[[nodiscard]] QBindable<qreal> bindableTimeout() { return &this->bTimeout; }
	[[nodiscard]] QBindable<bool> bindableRespectInhibitors() { return &this->bRespectInhibitors; }
	[[nodiscard]] QBindable<bool> bindableIsIdle() const { return &this->bIsIdle; }

signals:
	void enabledChanged();
	void timeoutChanged();
	void respectInhibitorsChanged();
	void isIdleChanged();

private:
	void updateNotification();

	struct Params {
		bool enabled;
		qreal timeout;
		bool respectInhibitors;
	};

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(IdleMonitor, bool, bEnabled, true, &IdleMonitor::enabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(IdleMonitor, qreal, bTimeout, &IdleMonitor::timeoutChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(IdleMonitor, bool, bRespectInhibitors, true, &IdleMonitor::respectInhibitorsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(IdleMonitor, Params, bParams, &IdleMonitor::updateNotification);
	Q_OBJECT_BINDABLE_PROPERTY(IdleMonitor, impl::IdleNotification*, bNotification);
	Q_OBJECT_BINDABLE_PROPERTY(IdleMonitor, bool, bIsIdle, &IdleMonitor::isIdleChanged);
	// clang-format on
};

} // namespace qs::wayland::idle_notify
