#include "shell_integration.hpp"

#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>

#include "layer_surface.hpp"
#include "wayland-wlr-layer-shell-unstable-v1-client-protocol.h"

QSWaylandLayerShellIntegration::QSWaylandLayerShellIntegration()
    : QtWaylandClient::QWaylandShellIntegrationTemplate<QSWaylandLayerShellIntegration>(4) {}

QSWaylandLayerShellIntegration::~QSWaylandLayerShellIntegration() {
	if (this->object() != nullptr) {
		zwlr_layer_shell_v1_destroy(this->object());
	}
}

QtWaylandClient::QWaylandShellSurface*
QSWaylandLayerShellIntegration::createShellSurface(QtWaylandClient::QWaylandWindow* window) {
	return new QSWaylandLayerSurface(this, window);
}
