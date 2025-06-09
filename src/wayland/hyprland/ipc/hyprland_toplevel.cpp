#include "hyprland_toplevel.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "toplevel_mapping.hpp"
#include "../../toplevel_management/handle.hpp"
#include "../../toplevel_management/qml.hpp"

using namespace qs::wayland::toplevel_management;
using namespace qs::wayland::toplevel_management::impl;

namespace qs::hyprland::ipc {

HyprlandToplevel::HyprlandToplevel(Toplevel* toplevel)
    : QObject(toplevel)
    , handle(toplevel->implHandle()) {
	auto* instance = HyprlandToplevelMappingManager::instance();
	auto addr = instance->getToplevelAddress(handle);

	if (addr != 0) this->setAddress(addr);
	else {
		QObject::connect(
		    instance,
		    &HyprlandToplevelMappingManager::toplevelAddressed,
		    this,
		    &HyprlandToplevel::onToplevelAddressed
		);
	}
}

void HyprlandToplevel::onToplevelAddressed(ToplevelHandle* handle, quint64 address) {
	if (handle == this->handle) {
		this->setAddress(address);
		QObject::disconnect(HyprlandToplevelMappingManager::instance(), nullptr, this, nullptr);
	}
}

void HyprlandToplevel::setAddress(quint64 address) {
	this->mAddress = QString::number(address, 16);
	emit this->addressChanged();
}

HyprlandToplevel* HyprlandToplevel::qmlAttachedProperties(QObject* object) {
	if (auto* toplevel = qobject_cast<Toplevel*>(object)) {
		return new HyprlandToplevel(toplevel);
	} else {
		return nullptr;
	}
}
} // namespace qs::hyprland::ipc
