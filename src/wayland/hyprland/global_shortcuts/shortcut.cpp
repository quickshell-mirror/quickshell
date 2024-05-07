#include "shortcut.hpp"

#include <qtmetamacros.h>
#include <qtypes.h>
#include <wayland-hyprland-global-shortcuts-v1-client-protocol.h>

namespace qs::hyprland::global_shortcuts::impl {

GlobalShortcut::GlobalShortcut(::hyprland_global_shortcut_v1* shortcut) { this->init(shortcut); }

GlobalShortcut::~GlobalShortcut() {
	if (this->isInitialized()) {
		this->destroy();
	}
}

void GlobalShortcut::hyprland_global_shortcut_v1_pressed(
    quint32 /*unused*/,
    quint32 /*unused*/,
    quint32 /*unused*/
) {
	emit this->pressed();
}

void GlobalShortcut::hyprland_global_shortcut_v1_released(
    quint32 /*unused*/,
    quint32 /*unused*/,
    quint32 /*unused*/
) {
	emit this->released();
}

} // namespace qs::hyprland::global_shortcuts::impl
