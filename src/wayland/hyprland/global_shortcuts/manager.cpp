#include "manager.hpp"

#include <qstring.h>
#include <qwaylandclientextension.h>

#include "shortcut.hpp"

namespace qs::hyprland::global_shortcuts::impl {

GlobalShortcutManager::GlobalShortcutManager()
    : QWaylandClientExtensionTemplate<GlobalShortcutManager>(1) {
	this->initialize();
}

GlobalShortcut* GlobalShortcutManager::registerShortcut(
    const QString& appid,
    const QString& name,
    const QString& description,
    const QString& triggerDescription
) {
	auto shortcut = this->shortcuts.value({appid, name});

	if (shortcut.second != nullptr) {
		this->shortcuts.insert({appid, name}, {shortcut.first + 1, shortcut.second});
		return shortcut.second;
	} else {
		auto* shortcutObj = this->register_shortcut(name, appid, description, triggerDescription);
		auto* managedObj = new GlobalShortcut(shortcutObj);
		this->shortcuts.insert({appid, name}, {1, managedObj});
		return managedObj;
	}
}

void GlobalShortcutManager::unregisterShortcut(const QString& appid, const QString& name) {
	auto shortcut = this->shortcuts.value({appid, name});

	if (shortcut.first > 1) {
		this->shortcuts.insert({appid, name}, {shortcut.first - 1, shortcut.second});
	} else {
		delete shortcut.second;
		this->shortcuts.remove({appid, name});
	}
}

GlobalShortcutManager* GlobalShortcutManager::instance() {
	static GlobalShortcutManager* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new GlobalShortcutManager();
	}

	return instance;
}

} // namespace qs::hyprland::global_shortcuts::impl
