#include "windowmanager.hpp"

namespace qs::wm::wayland {

WaylandWindowManager* WaylandWindowManager::instance() {
	static auto* instance = new WaylandWindowManager();
	return instance;
}

void installWmProvider() {
	qs::wm::WindowManager::setProvider([]() { return WaylandWindowManager::instance(); });
}

} // namespace qs::wm::wayland
