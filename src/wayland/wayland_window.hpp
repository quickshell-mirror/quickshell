#pragma once

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../window/proxywindow.hpp"

namespace qs::wayland {

///! Wayland specific QsWindow properties.
/// Allows setting wayland specific window properties on a @@Quickshell.QsWindow or subclass,
/// as an attached object.
///
/// #### Example
/// ```qml
/// @@Quickshell.FloatingWindow {
///   // ...
///   WaylandWindow.appId: "org.example.myapp"
/// }
/// ```
class WaylandWindow: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The [application id] of the window, used by the compositor to identify, group,
	/// and match windows against desktop entries. Only toplevel windows such as
	/// @@Quickshell.FloatingWindow have an application id.
	///
	/// If unset or empty, the app id of the quickshell instance is used.
	///
	/// > [!NOTE] The app id can be changed while the window is visible, but compositor
	/// > behaviors that were decided when the window was first shown, such as window
	/// > rules, may not be re-evaluated.
	///
	/// [application id]: https://wayland.app/protocols/xdg-shell#xdg_toplevel:request:set_app_id
	Q_PROPERTY(QString appId READ appId WRITE setAppId NOTIFY appIdChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("WaylandWindow can only be used as an attached object.");
	QML_ATTACHED(WaylandWindow);

public:
	explicit WaylandWindow(ProxyWindowBase* window);

	[[nodiscard]] QString appId() const;
	void setAppId(const QString& appId);

	static WaylandWindow* qmlAttachedProperties(QObject* object);

signals:
	void appIdChanged();

private slots:
	void onWindowConnected();
	void onWindowVisibleChanged();
	void onWaylandWindowDestroyed();
	void onSurfaceRoleCreated();

private:
	void applyAppId();

	ProxyWindowBase* proxyWindow = nullptr;
	QWindow* mWindow = nullptr;
	QtWaylandClient::QWaylandWindow* mWaylandWindow = nullptr;
	QString mAppId;
};

} // namespace qs::wayland
