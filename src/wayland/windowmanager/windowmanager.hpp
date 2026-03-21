#pragma once

#include <qtmetamacros.h>

#include "../../windowmanager/windowmanager.hpp"
#include "windowset.hpp"

namespace qs::wm::wayland {

class WaylandWindowManager: public WindowManager {
	Q_OBJECT;

public:
	static WaylandWindowManager* instance();
};

} // namespace qs::wm::wayland
