#pragma once

#include <qtmetamacros.h>

#include "../../windowmanager/windowmanager.hpp"
#include "workspace.hpp"

namespace qs::wm::wayland {

class WaylandWindowManager: public WindowManager {
	Q_OBJECT;

public:
	static WaylandWindowManager* instance();

	[[nodiscard]] UntypedObjectModel* workspaces() const override {
		return &WorkspaceManager::instance()->mWorkspaces;
	}

	[[nodiscard]] UntypedObjectModel* workspaceGroups() const override {
		return &WorkspaceManager::instance()->mWorkspaceGroups;
	}
};

} // namespace qs::wm::wayland
