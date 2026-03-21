#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"
#include "../core/popupanchor.hpp"
#include "../core/qmlscreen.hpp"
#include "proxywindow.hpp"
#include "windowinterface.hpp"

///! Popup window.
/// Popup window that can display in a position relative to a floating
/// or panel window.
///
/// #### Example
/// The following snippet creates a panel with a popup centered over it.
///
/// ```qml
/// PanelWindow {
///   id: toplevel
///
///   anchors {
///     bottom: true
///     left: true
///     right: true
///   }
///
///   PopupWindow {
///     anchor.window: toplevel
///     anchor.rect.x: parentWindow.width / 2 - width / 2
///     anchor.rect.y: parentWindow.height
///     width: 500
///     height: 500
///     visible: true
///   }
/// }
/// ```
class ProxyPopupWindow: public ProxyWindowBase {
	QSDOC_BASECLASS(WindowInterface);
	Q_OBJECT;
	// clang-format off
	/// > [!ERROR] Deprecated in favor of `anchor.window`.
	///
	/// The parent window of this popup.
	///
	/// Changing this property reparents the popup.
	Q_PROPERTY(QObject* parentWindow READ parentWindow WRITE setParentWindow NOTIFY parentWindowChanged);
	/// > [!ERROR] Deprecated in favor of `anchor.rect.x`.
	///
	/// The X position of the popup relative to the parent window.
	Q_PROPERTY(qint32 relativeX READ relativeX WRITE setRelativeX NOTIFY relativeXChanged);
	/// > [!ERROR] Deprecated in favor of `anchor.rect.y`.
	///
	/// The Y position of the popup relative to the parent window.
	Q_PROPERTY(qint32 relativeY READ relativeY WRITE setRelativeY NOTIFY relativeYChanged);
	/// The popup's anchor / positioner relative to another item or window. The popup will
	/// not be shown until it has a valid anchor relative to a window and @@visible is true.
	///
	/// You can set properties of the anchor like so:
	/// ```qml
	/// PopupWindow {
	///   anchor.window: parentwindow
	///   // or
	///   anchor {
	///     window: parentwindow
	///   }
	/// }
	/// ```
	Q_PROPERTY(PopupAnchor* anchor READ anchor CONSTANT);
	/// If the window is shown or hidden. Defaults to false.
	///
	/// The popup will not be shown until @@anchor is valid, regardless of this property.
	QSDOC_PROPERTY_OVERRIDE(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged);
	/// If true, the popup window will be dismissed and @@visible will change to false
	/// if the user clicks outside of the popup or it is otherwise closed.
	///
	/// > [!WARNING] Changes to this property while the window is open will only take
	/// > effect after the window is hidden and shown again.
	///
	/// > [!NOTE] Under Hyprland, @@Quickshell.Hyprland.HyprlandFocusGrab provides more advanced
	/// > functionality such as detecting clicks outside without closing the popup.
	Q_PROPERTY(bool grabFocus READ default WRITE default NOTIFY grabFocusChanged BINDABLE bindableGrabFocus);
	/// The screen that the window currently occupies.
	///
	/// This may be modified to move the window to the given screen.
	QSDOC_PROPERTY_OVERRIDE(QuickshellScreenInfo* screen READ screen NOTIFY screenChanged);
	// clang-format on
	QML_NAMED_ELEMENT(PopupWindow);

public:
	explicit ProxyPopupWindow(QObject* parent = nullptr);

	void completeWindow() override;
	void postCompleteWindow() override;
	void onPolished() override;
	bool deleteOnInvisible() const override;

	void setScreen(QuickshellScreenInfo* screen) override;
	void setVisible(bool visible) override;

	[[nodiscard]] QObject* parentWindow() const;
	void setParentWindow(QObject* parent);

	[[nodiscard]] qint32 relativeX() const;
	void setRelativeX(qint32 x);

	[[nodiscard]] qint32 relativeY() const;
	void setRelativeY(qint32 y);

	[[nodiscard]] QBindable<bool> bindableGrabFocus() { return &this->bWantsGrab; }

	[[nodiscard]] PopupAnchor* anchor();

signals:
	void parentWindowChanged();
	void relativeXChanged();
	void relativeYChanged();
	void grabFocusChanged();

private slots:
	void onParentWindowChanged();
	void onClosed();
	void reposition();

private:
	void targetVisibleChanged();

	QQuickWindow* parentBackingWindow();

	PopupAnchor mAnchor {this};
	bool pendingReposition = false;

	Q_OBJECT_BINDABLE_PROPERTY(ProxyPopupWindow, bool, bWantsVisible);

	Q_OBJECT_BINDABLE_PROPERTY(
	    ProxyPopupWindow,
	    bool,
	    bTargetVisible,
	    &ProxyPopupWindow::targetVisibleChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(ProxyPopupWindow, bool, bWantsGrab);
};
