#include "qml.hpp"

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
		this->mItems.insertObject(item);
	}
}

void SystemTray::onItemRegistered(StatusNotifierItem* item) { this->mItems.insertObject(item); }
void SystemTray::onItemUnregistered(StatusNotifierItem* item) { this->mItems.removeObject(item); }
ObjectModel<StatusNotifierItem>* SystemTray::items() { return &this->mItems; }
