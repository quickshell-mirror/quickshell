#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "doc.hpp"
#include "proxywindow.hpp"
#include "qmlscreen.hpp"
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
///     parentWindow: toplevel
///     relativeX: parentWindow.width / 2 - width / 2
///     relativeY: parentWindow.height
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
	/// The parent window of this popup.
	///
	/// Changing this property reparents the popup.
	Q_PROPERTY(QObject* parentWindow READ parentWindow WRITE setParentWindow NOTIFY parentWindowChanged);
	/// The X position of the popup relative to the parent window.
	Q_PROPERTY(qint32 relativeX READ relativeX WRITE setRelativeX NOTIFY relativeXChanged);
	/// The Y position of the popup relative to the parent window.
	Q_PROPERTY(qint32 relativeY READ relativeY WRITE setRelativeY NOTIFY relativeYChanged);
	/// If the window is shown or hidden. Defaults to false.
	QSDOC_PROPERTY_OVERRIDE(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged);
	/// The screen that the window currently occupies.
	///
	/// This may be modified to move the window to the given screen.
	QSDOC_PROPERTY_OVERRIDE(QuickshellScreenInfo* screen READ screen NOTIFY screenChanged);
	// clang-format on
	QML_NAMED_ELEMENT(PopupWindow);

public:
	explicit ProxyPopupWindow(QObject* parent = nullptr);

	void setupWindow() override;

	void setScreen(QuickshellScreenInfo* screen) override;
	void setVisible(bool visible) override;

	[[nodiscard]] qint32 x() const override;

	[[nodiscard]] QObject* parentWindow() const;
	void setParentWindow(QObject* parent);

	[[nodiscard]] qint32 relativeX() const;
	void setRelativeX(qint32 x);

	[[nodiscard]] qint32 relativeY() const;
	void setRelativeY(qint32 y);

signals:
	void parentWindowChanged();
	void relativeXChanged();
	void relativeYChanged();

private slots:
	void onParentConnected();
	void onParentDestroyed();
	void updateX();
	void updateY();

private:
	QQuickWindow* parentBackingWindow();
	void updateTransientParent();
	void updateVisible();

	QObject* mParentWindow = nullptr;
	ProxyWindowBase* mParentProxyWindow = nullptr;
	qint32 mRelativeX = 0;
	qint32 mRelativeY = 0;
	bool wantsVisible = false;
};
