#include "qml.hpp"

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/dbusmenu/dbusmenu.hpp"
#include "../../dbus/properties.hpp"
#include "host.hpp"
#include "item.hpp"

using namespace qs::dbus;
using namespace qs::dbus::dbusmenu;
using namespace qs::service::sni;

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

bool SystemTrayItem::onlyMenu() const {
	if (this->item == nullptr) return false;
	return this->item->isMenu.get();
}

void SystemTrayItem::activate() const { this->item->activate(); }
void SystemTrayItem::secondaryActivate() const { this->item->secondaryActivate(); }
void SystemTrayItem::scroll(qint32 delta, bool horizontal) const {
	this->item->scroll(delta, horizontal);
}

SystemTray::SystemTray(QObject* parent): QObject(parent) {
	auto* host = StatusNotifierHost::instance();

	// clang-format off
	QObject::connect(host, &StatusNotifierHost::itemReady, this, &SystemTray::onItemRegistered);
	QObject::connect(host, &StatusNotifierHost::itemUnregistered, this, &SystemTray::onItemUnregistered);
	// clang-format on

	for (auto* item: host->items()) {
		this->mItems.push_back(new SystemTrayItem(item, this));
	}
}

void SystemTray::onItemRegistered(StatusNotifierItem* item) {
	this->mItems.push_back(new SystemTrayItem(item, this));
	emit this->itemsChanged();
}

void SystemTray::onItemUnregistered(StatusNotifierItem* item) {
	SystemTrayItem* trayItem = nullptr;

	this->mItems.removeIf([item, &trayItem](SystemTrayItem* testItem) {
		if (testItem->item == item) {
			trayItem = testItem;
			return true;
		} else return false;
	});

	emit this->itemsChanged();

	delete trayItem;
}

QQmlListProperty<SystemTrayItem> SystemTray::items() {
	return QQmlListProperty<SystemTrayItem>(
	    this,
	    nullptr,
	    &SystemTray::itemsCount,
	    &SystemTray::itemAt
	);
}

qsizetype SystemTray::itemsCount(QQmlListProperty<SystemTrayItem>* property) {
	return reinterpret_cast<SystemTray*>(property->object)->mItems.count(); // NOLINT
}

SystemTrayItem* SystemTray::itemAt(QQmlListProperty<SystemTrayItem>* property, qsizetype index) {
	return reinterpret_cast<SystemTray*>(property->object)->mItems.at(index); // NOLINT
}

SystemTrayItem* SystemTrayMenuWatcher::trayItem() const { return this->item; }

void SystemTrayMenuWatcher::setTrayItem(SystemTrayItem* item) {
	if (item == this->item) return;

	if (this->item != nullptr) {
		QObject::disconnect(this->item, nullptr, this, nullptr);
	}

	this->item = item;

	if (item != nullptr) {
		QObject::connect(item, &QObject::destroyed, this, &SystemTrayMenuWatcher::onItemDestroyed);

		QObject::connect(
		    &item->item->menuPath,
		    &AbstractDBusProperty::changed,
		    this,
		    &SystemTrayMenuWatcher::onMenuPathChanged
		);
	}

	this->onMenuPathChanged();
	emit this->trayItemChanged();
}

DBusMenuItem* SystemTrayMenuWatcher::menu() const {
	if (this->mMenu == nullptr) return nullptr;
	return &this->mMenu->rootItem;
}

void SystemTrayMenuWatcher::onItemDestroyed() {
	this->item = nullptr;
	this->onMenuPathChanged();
	emit this->trayItemChanged();
}

void SystemTrayMenuWatcher::onMenuPathChanged() {
	if (this->mMenu != nullptr) {
		this->mMenu->deleteLater();
	}

	this->mMenu = this->item == nullptr ? nullptr : this->item->item->createMenu();
	emit this->menuChanged();
}
