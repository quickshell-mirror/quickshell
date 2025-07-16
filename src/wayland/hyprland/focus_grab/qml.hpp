#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

class ProxyWindowBase;

namespace qs::hyprland {

namespace focus_grab {
class FocusGrab;
}

///! Input focus grabber
/// Object for managing input focus grabs via the [hyprland_focus_grab_v1]
/// wayland protocol.
///
/// When enabled, all of the windows listed in the `windows` property will
/// receive input normally, and will retain keyboard focus even if the mouse
/// is moved off of them. When areas of the screen that are not part of a listed
/// window are clicked or touched, the grab will become inactive and emit the
/// cleared signal.
///
/// This is useful for implementing dismissal of popup type windows.
/// ```qml
/// import Quickshell
/// import Quickshell.Hyprland
/// import QtQuick.Controls
///
/// ShellRoot {
///   FloatingWindow {
///     id: window
///
///     Button {
///       anchors.centerIn: parent
///       text: grab.active ? "Remove exclusive focus" : "Take exclusive focus"
///       onClicked: grab.active = !grab.active
///     }
///
///     HyprlandFocusGrab {
///       id: grab
///       windows: [ window ]
///     }
///   }
/// }
/// ```
///
/// [hyprland_focus_grab_v1]: https://github.com/hyprwm/hyprland-protocols/blob/main/protocols/hyprland-global-shortcuts-v1.xml
class HyprlandFocusGrab
    : public QObject
    , public QQmlParserStatus {
	Q_OBJECT;
	QML_ELEMENT;
	Q_INTERFACES(QQmlParserStatus);
	/// If the focus grab is active. Defaults to false.
	///
	/// When set to true, an input grab will be created for the listed windows.
	///
	/// This property will change to false once the grab is dismissed.
	/// It will not change to true until the grab begins, which requires
	/// at least one visible window.
	Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged);
	/// The list of windows to whitelist for input.
	Q_PROPERTY(QList<QObject*> windows READ windows WRITE setWindows NOTIFY windowsChanged);

public:
	explicit HyprlandFocusGrab(QObject* parent = nullptr): QObject(parent) {}

	void classBegin() override {}
	void componentComplete() override;

	[[nodiscard]] bool isActive() const;
	void setActive(bool active);

	[[nodiscard]] QObjectList windows() const;
	void setWindows(QObjectList windows);

signals:
	/// Sent whenever the compositor clears the focus grab.
	///
	/// This may be in response to all windows being removed
	/// from the list or simultaneously hidden, in addition to
	/// a normal clear.
	void cleared();

	void activeChanged();
	void windowsChanged();

private slots:
	void onGrabActivated();
	void onGrabCleared();
	void onProxyConnected();

private:
	void tryActivate();
	void syncWindows();

	bool targetActive = false;
	QObjectList windowObjects;
	QList<ProxyWindowBase*> trackedProxies;
	QList<QWindow*> trackedWindows;

	focus_grab::FocusGrab* grab = nullptr;
};

}; // namespace qs::hyprland
