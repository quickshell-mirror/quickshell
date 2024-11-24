#include "qml.hpp"
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "manager.hpp"

namespace qs::hyprland::global_shortcuts {
using impl::GlobalShortcutManager;

GlobalShortcut::~GlobalShortcut() {
	auto* manager = GlobalShortcutManager::instance();
	if (manager != nullptr) {
		manager->unregisterShortcut(this->mAppid, this->mName);
	}
}

void GlobalShortcut::onPostReload() {
	if (this->mName.isEmpty()) {
		qWarning() << "Unable to create GlobalShortcut with empty name.";
		return;
	}

	auto* manager = GlobalShortcutManager::instance();
	if (!manager->isActive()) {
		qWarning() << "The active compositor does not support hyprland_global_shortcuts_v1.";
		qWarning() << "GlobalShortcut will not work.";
		return;
	}

	this->shortcut = manager->registerShortcut(
	    this->mAppid,
	    this->mName,
	    this->mDescription,
	    this->mTriggerDescription
	);

	QObject::connect(this->shortcut, &ShortcutImpl::pressed, this, &GlobalShortcut::onPressed);
	QObject::connect(this->shortcut, &ShortcutImpl::released, this, &GlobalShortcut::onReleased);
}

bool GlobalShortcut::isPressed() const { return this->mPressed; }

QString GlobalShortcut::appid() const { return this->mAppid; }

void GlobalShortcut::setAppid(QString appid) {
	if (this->shortcut != nullptr) {
		qWarning() << "GlobalShortcut cannot be modified after creation.";
		return;
	}

	if (appid == this->mAppid) return;

	this->mAppid = std::move(appid);
	emit this->appidChanged();
}

QString GlobalShortcut::name() const { return this->mName; }

void GlobalShortcut::setName(QString name) {
	if (this->shortcut != nullptr) {
		qWarning() << "GlobalShortcut cannot be modified after creation.";
		return;
	}

	if (name == this->mName) return;

	this->mName = std::move(name);
	emit this->nameChanged();
}

QString GlobalShortcut::description() const { return this->mDescription; }

void GlobalShortcut::setDescription(QString description) {
	if (this->shortcut != nullptr) {
		qWarning() << "GlobalShortcut cannot be modified after creation.";
		return;
	}

	if (description == this->mDescription) return;

	this->mDescription = std::move(description);
	emit this->descriptionChanged();
}

QString GlobalShortcut::triggerDescription() const { return this->mTriggerDescription; }

void GlobalShortcut::setTriggerDescription(QString triggerDescription) {
	if (this->shortcut != nullptr) {
		qWarning() << "GlobalShortcut cannot be modified after creation.";
		return;
	}

	if (triggerDescription == this->mTriggerDescription) return;

	this->mTriggerDescription = std::move(triggerDescription);
	emit this->triggerDescriptionChanged();
}

void GlobalShortcut::onPressed() {
	this->mPressed = true;
	emit this->pressed();
	emit this->pressedChanged();
}

void GlobalShortcut::onReleased() {
	this->mPressed = false;
	emit this->released();
	emit this->pressedChanged();
}

} // namespace qs::hyprland::global_shortcuts
