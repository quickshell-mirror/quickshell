#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

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
class ShortcutsInhibitor: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// If the shortcuts inhibitor should be enabled. Defaults to false.
	Q_PROPERTY(bool enabled READ default WRITE default NOTIFY enabledChanged BINDABLE bindableEnabled);
	/// The window to associate the shortcuts inhibitor with.
	///
  /// Must be set to a non null value to enable the inhibitor.
	Q_PROPERTY(QObject* window READ window WRITE setWindow NOTIFY windowChanged);
	// clang-format on

public:
	ShortcutsInhibitor();
	~ShortcutsInhibitor() override;
	Q_DISABLE_COPY_MOVE(ShortcutsInhibitor);

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

	impl::ShortcutsInhibitor* inhibitor = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutsInhibitor, bool, bEnabled, &ShortcutsInhibitor::enabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutsInhibitor, QObject*, bWindowObject, &ShortcutsInhibitor::windowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutsInhibitor, QObject*, bBoundWindow, &ShortcutsInhibitor::boundWindowChanged);
	// clang-format on
};

} // namespace qs::wayland::shortcuts_inhibit