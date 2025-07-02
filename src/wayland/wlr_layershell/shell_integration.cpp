#include "shell_integration.hpp"

#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>

#include "surface.hpp"

namespace qs::wayland::layershell {

LayerShellIntegration::LayerShellIntegration()
    : QtWaylandClient::QWaylandShellIntegrationTemplate<LayerShellIntegration>(4) {}

LayerShellIntegration::~LayerShellIntegration() {
	if (this->isInitialized()) {
		this->destroy();
	}
}

QtWaylandClient::QWaylandShellSurface*
LayerShellIntegration::createShellSurface(QtWaylandClient::QWaylandWindow* window) {
	return new LayerSurface(this, window);
}

} // namespace qs::wayland::layershell
