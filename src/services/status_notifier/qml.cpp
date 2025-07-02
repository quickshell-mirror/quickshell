#include "qml.hpp"

#include <qnamespace.h>
#include <qobject.h>

#include "../../core/model.hpp"
#include "host.hpp"
#include "item.hpp"

using namespace qs::service::sni;

SystemTray::SystemTray(QObject* parent): QObject(parent) {
	auto* host = StatusNotifierHost::instance();

	// clang-format off
	QObject::connect(host, &StatusNotifierHost::itemReady, this, &SystemTray::onItemRegistered);
	QObject::connect(host, &StatusNotifierHost::itemUnregistered, this, &SystemTray::onItemUnregistered);
	// clang-format on

	for (auto* item: host->items()) {
		this->mItems.insertObjectSorted(item, &SystemTray::compareItems);
	}
}

void SystemTray::onItemRegistered(StatusNotifierItem* item) {
	this->mItems.insertObjectSorted(item, &SystemTray::compareItems);
}

void SystemTray::onItemUnregistered(StatusNotifierItem* item) { this->mItems.removeObject(item); }
ObjectModel<StatusNotifierItem>* SystemTray::items() { return &this->mItems; }

bool SystemTray::compareItems(StatusNotifierItem* a, StatusNotifierItem* b) {
	return a->bindableCategory().value() < b->bindableCategory().value()
	    || a->bindableId().value().compare(b->bindableId().value(), Qt::CaseInsensitive) >= 0;
}
