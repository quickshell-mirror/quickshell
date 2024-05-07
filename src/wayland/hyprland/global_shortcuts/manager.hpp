#pragma once

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qstring.h>
#include <qwayland-hyprland-global-shortcuts-v1.h>
#include <qwaylandclientextension.h>

#include "shortcut.hpp"

namespace qs::hyprland::global_shortcuts::impl {

class GlobalShortcutManager
    : public QWaylandClientExtensionTemplate<GlobalShortcutManager>
    , public QtWayland::hyprland_global_shortcuts_manager_v1 {
public:
	explicit GlobalShortcutManager();

	GlobalShortcut* registerShortcut(
	    const QString& appid,
	    const QString& name,
	    const QString& description,
	    const QString& triggerDescription
	);

	void unregisterShortcut(const QString& appid, const QString& name);

	static GlobalShortcutManager* instance();

private:
	QHash<QPair<QString, QString>, QPair<qint32, GlobalShortcut*>> shortcuts;
};

} // namespace qs::hyprland::global_shortcuts::impl
