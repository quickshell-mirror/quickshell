#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../../window/proxywindow.hpp"
#include "proto.hpp"

namespace qs::wayland::shortcuts_inhibit {

///! Prevents compositor keyboard shortcuts from being triggered
/// A shortcuts inhibitor prevents the compositor from processing its own keyboard shortcuts
/// for the focused surface. This allows applications to receive key events for shortcuts
/// that would normally be handled by the compositor.
///
/// The inhibitor only takes effect when the associated window is focused and the inhibitor
/// is enabled. The compositor may choose to ignore inhibitor requests based on its policy.
///
/// > [!NOTE] Using a shortcuts inhibitor requires the compositor support the [keyboard-shortcuts-inhibit-unstable-v1] protocol.
///
/// [keyboard-shortcuts-inhibit-unstable-v1]: https://wayland.app/protocols/keyboard-shortcuts-inhibit-unstable-v1
class ShortcutInhibitor: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// If the shortcuts inhibitor should be enabled. Defaults to false.
	Q_PROPERTY(bool enabled READ default WRITE default NOTIFY enabledChanged BINDABLE bindableEnabled);
	/// The window to associate the shortcuts inhibitor with.
  /// The inhibitor will only inhibit shortcuts pressed while this window has keyboard focus.
	///
  /// Must be set to a non null value to enable the inhibitor.
	Q_PROPERTY(QObject* window READ default WRITE default NOTIFY windowChanged BINDABLE bindableWindow);
	/// Whether the inhibitor is currently active. The inhibitor is only active if @@enabled is true,
  /// @@window has keyboard focus, and the compositor grants the inhibit request.
	///
	/// The compositor may deactivate the inhibitor at any time (for example, if the user requests
	/// normal shortcuts to be restored). When deactivated by the compositor, the inhibitor cannot be
	/// programmatically reactivated.
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	// clang-format on

public:
	ShortcutInhibitor();
	~ShortcutInhibitor() override;
	Q_DISABLE_COPY_MOVE(ShortcutInhibitor);

	[[nodiscard]] QBindable<bool> bindableEnabled() { return &this->bEnabled; }
	[[nodiscard]] QBindable<QObject*> bindableWindow() { return &this->bWindowObject; }
	[[nodiscard]] QBindable<bool> bindableActive() const { return &this->bActive; }

signals:
	void enabledChanged();
	void windowChanged();
	void activeChanged();
	/// Sent if the compositor cancels the inhibitor while it is active.
	void cancelled();

private slots:
	void onWindowDestroyed();
	void onWindowVisibilityChanged();
	void onWaylandWindowDestroyed();
	void onWaylandSurfaceCreated();
	void onWaylandSurfaceDestroyed();
	void onInhibitorActiveChanged();

private:
	void onBoundWindowChanged();
	void onInhibitorChanged();
	void onFocusedWindowChanged(QWindow* focusedWindow);

	ProxyWindowBase* proxyWindow = nullptr;
	QtWaylandClient::QWaylandWindow* mWaylandWindow = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, bool, bEnabled, &ShortcutInhibitor::enabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, QObject*, bWindowObject, &ShortcutInhibitor::windowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, QObject*, bBoundWindow, &ShortcutInhibitor::onBoundWindowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, impl::ShortcutsInhibitor*, bInhibitor, &ShortcutInhibitor::onInhibitorChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, QWindow*, bWindow);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, QWindow*, bFocusedWindow);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutInhibitor, bool, bActive, &ShortcutInhibitor::activeChanged);
	// clang-format on
};

} // namespace qs::wayland::shortcuts_inhibit
