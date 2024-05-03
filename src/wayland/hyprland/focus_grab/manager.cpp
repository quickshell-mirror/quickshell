#include "manager.hpp"

#include <qwaylandclientextension.h>

#include "grab.hpp"

namespace qs::hyprland::focus_grab {

FocusGrabManager::FocusGrabManager(): QWaylandClientExtensionTemplate<FocusGrabManager>(1) {
	this->initialize();
}

bool FocusGrabManager::available() const { return this->isActive(); }

FocusGrab* FocusGrabManager::createGrab() { return new FocusGrab(this->create_grab()); }

FocusGrabManager* FocusGrabManager::instance() {
	static FocusGrabManager* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new FocusGrabManager();
	}

	return instance;
}

} // namespace qs::hyprland::focus_grab
