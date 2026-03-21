#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../../window/proxywindow.hpp"
#include "proto.hpp"

namespace qs::wayland::idle_inhibit {

///! Prevents a wayland session from idling
/// If an idle daemon is running, it may perform actions such as locking the screen
/// or putting the computer to sleep.
///
/// An idle inhibitor prevents a wayland session from being marked as idle, if compositor
/// defined heuristics determine the window the inhibitor is attached to is important.
///
/// A compositor will usually consider a @@Quickshell.PanelWindow or
/// a focused @@Quickshell.FloatingWindow to be important.
///
/// > [!NOTE] Using an idle inhibitor requires the compositor support the [idle-inhibit-unstable-v1] protocol.
///
/// [idle-inhibit-unstable-v1]: https://wayland.app/protocols/idle-inhibit-unstable-v1
class IdleInhibitor: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// If the idle inhibitor should be enabled. Defaults to false.
	Q_PROPERTY(bool enabled READ default WRITE default NOTIFY enabledChanged BINDABLE bindableEnabled);
	/// The window to associate the idle inhibitor with. This may be used by the compositor
	/// to determine if the inhibitor should be respected.
	///
  /// Must be set to a non null value to enable the inhibitor.
	Q_PROPERTY(QObject* window READ window WRITE setWindow NOTIFY windowChanged);
	// clang-format on

public:
	IdleInhibitor();
	~IdleInhibitor() override;
	Q_DISABLE_COPY_MOVE(IdleInhibitor);

	[[nodiscard]] QObject* window() const;
	void setWindow(QObject* window);

	[[nodiscard]] QBindable<bool> bindableEnabled() { return &this->bEnabled; }

signals:
	void enabledChanged();
	void windowChanged();

private slots:
	void onWindowDestroyed();
	void onWindowVisibilityChanged();
	void onWaylandWindowDestroyed();
	void onWaylandSurfaceCreated();
	void onWaylandSurfaceDestroyed();

private:
	void boundWindowChanged();
	ProxyWindowBase* proxyWindow = nullptr;
	QtWaylandClient::QWaylandWindow* mWaylandWindow = nullptr;

	impl::IdleInhibitor* inhibitor = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(IdleInhibitor, bool, bEnabled, &IdleInhibitor::enabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(IdleInhibitor, QObject*, bWindowObject, &IdleInhibitor::windowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(IdleInhibitor, QObject*, bBoundWindow, &IdleInhibitor::boundWindowChanged);
	// clang-format on
};

} // namespace qs::wayland::idle_inhibit
