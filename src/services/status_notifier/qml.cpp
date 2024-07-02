#include "qml.hpp"

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/model.hpp"
#include "../../core/platformmenu.hpp"
#include "../../dbus/dbusmenu/dbusmenu.hpp"
#include "../../dbus/properties.hpp"
#include "host.hpp"
#include "item.hpp"

using namespace qs::dbus;
using namespace qs::dbus::dbusmenu;
using namespace qs::service::sni;
using namespace qs::menu::platform;

SystemTrayItem::SystemTrayItem(qs::service::sni::StatusNotifierItem* item, QObject* parent)
    : QObject(parent)
    , item(item) {
	// clang-format off
	QObject::connect(&this->item->id, &AbstractDBusProperty::changed, this, &SystemTrayItem::idChanged);
	QObject::connect(&this->item->title, &AbstractDBusProperty::changed, this, &SystemTrayItem::titleChanged);
	QObject::connect(&this->item->status, &AbstractDBusProperty::changed, this, &SystemTrayItem::statusChanged);
	QObject::connect(&this->item->category, &AbstractDBusProperty::changed, this, &SystemTrayItem::categoryChanged);
	QObject::connect(this->item, &StatusNotifierItem::iconChanged, this, &SystemTrayItem::iconChanged);
	QObject::connect(&this->item->tooltip, &AbstractDBusProperty::changed, this, &SystemTrayItem::tooltipTitleChanged);
	QObject::connect(&this->item->tooltip, &AbstractDBusProperty::changed, this, &SystemTrayItem::tooltipDescriptionChanged);
	QObject::connect(&this->item->menuPath, &AbstractDBusProperty::changed, this, &SystemTrayItem::hasMenuChanged);
	QObject::connect(&this->item->isMenu, &AbstractDBusProperty::changed, this, &SystemTrayItem::onlyMenuChanged);
	// clang-format on
}

QString SystemTrayItem::id() const {
	if (this->item == nullptr) return "";
	return this->item->id.get();
}

QString SystemTrayItem::title() const {
	if (this->item == nullptr) return "";
	return this->item->title.get();
}

SystemTrayStatus::Enum SystemTrayItem::status() const {
	if (this->item == nullptr) return SystemTrayStatus::Passive;
	auto status = this->item->status.get();

	if (status == "Passive") return SystemTrayStatus::Passive;
	if (status == "Active") return SystemTrayStatus::Active;
	if (status == "NeedsAttention") return SystemTrayStatus::NeedsAttention;

	qCWarning(logStatusNotifierItem) << "Nonconformant StatusNotifierItem status" << status
	                                 << "returned for" << this->item->properties.toString();

	return SystemTrayStatus::Passive;
}

SystemTrayCategory::Enum SystemTrayItem::category() const {
	if (this->item == nullptr) return SystemTrayCategory::ApplicationStatus;
	auto category = this->item->category.get();

	if (category == "ApplicationStatus") return SystemTrayCategory::ApplicationStatus;
	if (category == "SystemServices") return SystemTrayCategory::SystemServices;
	if (category == "Hardware") return SystemTrayCategory::Hardware;

	qCWarning(logStatusNotifierItem) << "Nonconformant StatusNotifierItem category" << category
	                                 << "returned for" << this->item->properties.toString();

	return SystemTrayCategory::ApplicationStatus;
}

QString SystemTrayItem::icon() const {
	if (this->item == nullptr) return "";
	return this->item->iconId();
}

QString SystemTrayItem::tooltipTitle() const {
	if (this->item == nullptr) return "";
	return this->item->tooltip.get().title;
}

QString SystemTrayItem::tooltipDescription() const {
	if (this->item == nullptr) return "";
	return this->item->tooltip.get().description;
}

bool SystemTrayItem::hasMenu() const {
	if (this->item == nullptr) return false;
	return !this->item->menuPath.get().path().isEmpty();
}

bool SystemTrayItem::onlyMenu() const {
	if (this->item == nullptr) return false;
	return this->item->isMenu.get();
}

void SystemTrayItem::activate() const { this->item->activate(); }
void SystemTrayItem::secondaryActivate() const { this->item->secondaryActivate(); }

void SystemTrayItem::scroll(qint32 delta, bool horizontal) const {
	this->item->scroll(delta, horizontal);
}

void SystemTrayItem::display(QObject* parentWindow, qint32 relativeX, qint32 relativeY) {
	this->item->refMenu();
	if (!this->item->menu()) {
		this->item->unrefMenu();
		qCritical() << "No menu present for" << this;
		return;
	}

	auto* platform = new PlatformMenuEntry(&this->item->menu()->rootItem);

	QObject::connect(&this->item->menu()->rootItem, &DBusMenuItem::layoutUpdated, platform, [=]() {
		platform->relayout();
		auto success = platform->display(parentWindow, relativeX, relativeY);

		// calls destroy which also unrefs
		if (!success) delete platform;
	});

	QObject::connect(platform, &PlatformMenuEntry::closed, this, [=]() { platform->deleteLater(); });
	QObject::connect(platform, &QObject::destroyed, this, [this]() { this->item->unrefMenu(); });
}

SystemTray::SystemTray(QObject* parent): QObject(parent) {
	auto* host = StatusNotifierHost::instance();

	// clang-format off
	QObject::connect(host, &StatusNotifierHost::itemReady, this, &SystemTray::onItemRegistered);
	QObject::connect(host, &StatusNotifierHost::itemUnregistered, this, &SystemTray::onItemUnregistered);
	// clang-format on

	for (auto* item: host->items()) {
		this->mItems.insertObject(new SystemTrayItem(item, this));
	}
}

void SystemTray::onItemRegistered(StatusNotifierItem* item) {
	this->mItems.insertObject(new SystemTrayItem(item, this));
}

void SystemTray::onItemUnregistered(StatusNotifierItem* item) {
	for (const auto* storedItem: this->mItems.valueList()) {
		if (storedItem->item == item) {
			this->mItems.removeObject(storedItem);
			delete storedItem;
			break;
		}
	}
}

ObjectModel<SystemTrayItem>* SystemTray::items() { return &this->mItems; }

SystemTrayItem* SystemTrayMenuWatcher::trayItem() const { return this->item; }

SystemTrayMenuWatcher::~SystemTrayMenuWatcher() {
	if (this->item != nullptr) {
		this->item->item->unrefMenu();
	}
}

void SystemTrayMenuWatcher::setTrayItem(SystemTrayItem* item) {
	if (item == this->item) return;

	if (this->item != nullptr) {
		this->item->item->unrefMenu();
		QObject::disconnect(this->item, nullptr, this, nullptr);
	}

	this->item = item;

	if (item != nullptr) {
		this->item->item->refMenu();

		QObject::connect(item, &QObject::destroyed, this, &SystemTrayMenuWatcher::onItemDestroyed);

		QObject::connect(
		    item->item,
		    &StatusNotifierItem::menuChanged,
		    this,
		    &SystemTrayMenuWatcher::menuChanged
		);
	}

	emit this->trayItemChanged();
	emit this->menuChanged();
}

DBusMenuItem* SystemTrayMenuWatcher::menu() const {
	return this->item ? &this->item->item->menu()->rootItem : nullptr;
}

void SystemTrayMenuWatcher::onItemDestroyed() {
	this->item = nullptr;
	emit this->trayItemChanged();
	emit this->menuChanged();
}
