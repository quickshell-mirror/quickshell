#include "toplevel_mapping.hpp"

#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwaylandclientextension.h>

#include "../../toplevel_management/manager.hpp"

using namespace qs::wayland::toplevel_management::impl;

namespace qs::hyprland::ipc {

HyprlandToplevelMappingHandle::~HyprlandToplevelMappingHandle() {
	if (this->isInitialized()) this->destroy();
}

void HyprlandToplevelMappingHandle::hyprland_toplevel_window_mapping_handle_v1_window_address(
    quint32 addressHi,
    quint32 addressLo
) {
	auto address = static_cast<quint64>(addressHi) << 32 | addressLo;
	HyprlandToplevelMappingManager::instance()->assignAddress(this->handle, address);
	delete this;
}

void HyprlandToplevelMappingHandle::hyprland_toplevel_window_mapping_handle_v1_failed() {
	delete this;
}

HyprlandToplevelMappingManager::HyprlandToplevelMappingManager()
    : QWaylandClientExtensionTemplate(1) {
	this->initialize();

	if (!this->isInitialized()) {
		qWarning() << "Compositor does not support hyprland-toplevel-mapping-v1."
		              "It will not be possible to derive hyprland addresses from toplevels.";
		return;
	}

	QObject::connect(
	    ToplevelManager::instance(),
	    &ToplevelManager::toplevelReady,
	    this,
	    &HyprlandToplevelMappingManager::onToplevelReady
	);

	for (auto* toplevel: ToplevelManager::instance()->readyToplevels()) {
		this->onToplevelReady(toplevel);
	}
}

void HyprlandToplevelMappingManager::onToplevelReady(ToplevelHandle* handle) {
	QObject::connect(
	    handle,
	    &QObject::destroyed,
	    this,
	    &HyprlandToplevelMappingManager::onToplevelDestroyed
	);

	new HyprlandToplevelMappingHandle(handle, this->get_window_for_toplevel_wlr(handle->object()));
}

void HyprlandToplevelMappingManager::assignAddress(ToplevelHandle* handle, quint64 address) {
	this->addresses.insert(handle, address);
	emit this->toplevelAddressed(handle, address);
}

void HyprlandToplevelMappingManager::onToplevelDestroyed(QObject* object) {
	this->addresses.remove(static_cast<ToplevelHandle*>(object)); // NOLINT
}

quint64 HyprlandToplevelMappingManager::getToplevelAddress(ToplevelHandle* handle) const {
	return this->addresses.value(handle);
}

HyprlandToplevelMappingManager* HyprlandToplevelMappingManager::instance() {
	static auto* instance = new HyprlandToplevelMappingManager();
	return instance;
}

} // namespace qs::hyprland::ipc
