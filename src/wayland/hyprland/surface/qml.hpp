#pragma once

#include <memory>

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "../../../core/region.hpp"
#include "../../../window/proxywindow.hpp"
#include "surface.hpp"

namespace qs::hyprland::surface {

///! Hyprland specific QsWindow properties.
/// Allows setting hyprland specific window properties on a @@Quickshell.QsWindow or subclass,
/// as an attached object.
///
/// #### Example
/// ```qml
/// @@Quickshell.PopupWindow {
///   // ...
///   HyprlandWindow.opacity: 0.6 // any number or binding
/// }
/// ```
///
/// > [!NOTE] Requires at least hyprland 0.47.0, or [hyprland-surface-v1] support.
///
/// [hyprland-surface-v1]: https://github.com/hyprwm/hyprland-protocols/blob/main/protocols/hyprland-surface-v1.xml
class HyprlandWindow: public QObject {
	Q_OBJECT;
	// clang-format off
	/// A multiplier for the window's overall opacity, ranging from 1.0 to 0.0. Overall opacity includes the opacity of
	/// both the window content *and* visual effects such as blur that apply to it.
	///
	/// Default: 1.0
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged);
	/// A hint to the compositor that only certain regions of the surface should be rendered.
	/// This can be used to avoid rendering large empty regions of a window which can increase
	/// performance, especially if the window is blurred. The mask should include all pixels
	/// of the window that do not have an alpha value of 0.
	Q_PROPERTY(PendingRegion* visibleMask READ visibleMask WRITE setVisibleMask NOTIFY visibleMaskChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("HyprlandWindow can only be used as an attached object.");
	QML_ATTACHED(HyprlandWindow);

public:
	explicit HyprlandWindow(ProxyWindowBase* window);

	[[nodiscard]] static bool available();

	[[nodiscard]] qreal opacity() const;
	void setOpacity(qreal opacity);

	[[nodiscard]] PendingRegion* visibleMask() const;
	virtual void setVisibleMask(PendingRegion* mask);

	static HyprlandWindow* qmlAttachedProperties(QObject* object);

signals:
	void opacityChanged();
	void visibleMaskChanged();

private slots:
	void onWindowConnected();
	void onWindowVisibleChanged();
	void onWaylandWindowDestroyed();
	void onWaylandSurfaceCreated();
	void onWaylandSurfaceDestroyed();
	void onProxyWindowDestroyed();
	void onVisibleMaskDestroyed();
	void onWindowPolished();
	void updateVisibleMask();

private:
	void disconnectWaylandWindow();

	ProxyWindowBase* proxyWindow = nullptr;
	QWindow* mWindow = nullptr;
	QtWaylandClient::QWaylandWindow* mWaylandWindow = nullptr;

	struct {
		bool opacity : 1 = false;
		bool visibleMask : 1 = false;
	} pendingPolish;

	qreal mOpacity = 1.0;
	PendingRegion* mVisibleMask = nullptr;
	std::unique_ptr<impl::HyprlandSurface> surface;
};

} // namespace qs::hyprland::surface
