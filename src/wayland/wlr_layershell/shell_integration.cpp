#include "shell_integration.hpp"

#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>

#include "surface.hpp"

QSWaylandLayerShellIntegration::QSWaylandLayerShellIntegration()
    : QtWaylandClient::QWaylandShellIntegrationTemplate<QSWaylandLayerShellIntegration>(4) {}

QSWaylandLayerShellIntegration::~QSWaylandLayerShellIntegration() {
	if (this->isInitialized()) {
		this->destroy();
	}
}

QtWaylandClient::QWaylandShellSurface*
QSWaylandLayerShellIntegration::createShellSurface(QtWaylandClient::QWaylandWindow* window) {
	return new QSWaylandLayerSurface(this, window);
}
