#include "windowmanager.hpp"

#include "../../windowmanager/windowmanager.hpp"
#include "windowset.hpp"

namespace qs::wm::wayland {

WaylandWindowManager* WaylandWindowManager::instance() {
	static auto* instance = []() {
		auto* wm = new WaylandWindowManager();
		WindowsetManager::instance();
		return wm;
	}();
	return instance;
}

void installWmProvider() { // NOLINT (misc-use-internal-linkage)
	qs::wm::WindowManager::setProvider([]() { return WaylandWindowManager::instance(); });
}

} // namespace qs::wm::wayland
