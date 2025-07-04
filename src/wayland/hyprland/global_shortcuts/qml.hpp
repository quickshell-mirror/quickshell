#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../../../core/reload.hpp"
#include "shortcut.hpp"

namespace qs::hyprland::global_shortcuts {

///! Hyprland global shortcut.
/// Global shortcut implemented with [hyprland_global_shortcuts_v1].
///
/// You can use this within hyprland as a global shortcut:
/// ```
/// bind = <modifiers>, <key>, global, <appid>:<name>
/// ```
/// See [the wiki] for details.
///
/// > [!WARNING] The shortcuts protocol does not allow duplicate appid + name pairs.
/// > Within a single instance of quickshell this is handled internally, and both
/// > users will be notified, but multiple instances of quickshell or XDPH may collide.
/// >
/// > If that happens, whichever client that tries to register the shortcuts last will crash.
///
/// > [!INFO] This type does *not* use the xdg-desktop-portal global shortcuts protocol,
/// > as it is not fully functional without flatpak and would cause a considerably worse
/// > user experience from other limitations. It will only work with Hyprland.
/// > Note that, as this type bypasses xdg-desktop-portal, XDPH is not required.
///
/// [hyprland_global_shortcuts_v1]: https://github.com/hyprwm/hyprland-protocols/blob/main/protocols/hyprland-global-shortcuts-v1.xml
/// [the wiki]: https://wiki.hyprland.org/Configuring/Binds/#dbus-global-shortcuts
class GlobalShortcut: public PostReloadHook {
	using ShortcutImpl = impl::GlobalShortcut;

	Q_OBJECT;
	// clang-format off
	/// If the keybind is currently pressed.
	Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged);
	/// The appid of the shortcut. Defaults to `quickshell`.
	/// You cannot change this at runtime.
	///
	/// If you have more than one shortcut we recommend subclassing
	/// GlobalShortcut to set this.
	Q_PROPERTY(QString appid READ appid WRITE setAppid NOTIFY appidChanged);
	/// The name of the shortcut.
	/// You cannot change this at runtime.
	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
	/// The description of the shortcut that appears in `hyprctl globalshortcuts`.
	/// You cannot change this at runtime.
	Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
	/// Have not seen this used ever, but included for completeness. Safe to ignore.
	Q_PROPERTY(QString triggerDescription READ triggerDescription WRITE setTriggerDescription NOTIFY triggerDescriptionChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit GlobalShortcut(QObject* parent = nullptr): PostReloadHook(parent) {}
	~GlobalShortcut() override;
	Q_DISABLE_COPY_MOVE(GlobalShortcut);

	void onPostReload() override;

	[[nodiscard]] bool isPressed() const;

	[[nodiscard]] QString appid() const;
	void setAppid(QString appid);

	[[nodiscard]] QString name() const;
	void setName(QString name);

	[[nodiscard]] QString description() const;
	void setDescription(QString description);

	[[nodiscard]] QString triggerDescription() const;
	void setTriggerDescription(QString triggerDescription);

signals:
	/// Fired when the keybind is pressed.
	void pressed();
	/// Fired when the keybind is released.
	void released();

	void pressedChanged();
	void appidChanged();
	void nameChanged();
	void descriptionChanged();
	void triggerDescriptionChanged();

private slots:
	void onPressed();
	void onReleased();

private:
	impl::GlobalShortcut* shortcut = nullptr;

	bool mPressed = false;
	QString mAppid = "quickshell";
	QString mName;
	QString mDescription;
	QString mTriggerDescription;
};

} // namespace qs::hyprland::global_shortcuts
