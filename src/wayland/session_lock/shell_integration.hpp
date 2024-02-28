#pragma once

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>

class QSWaylandSessionLockIntegration: public QtWaylandClient::QWaylandShellIntegration {
public:
	bool initialize(QtWaylandClient::QWaylandDisplay* /* display */) override { return true; }
	QtWaylandClient::QWaylandShellSurface* createShellSurface(QtWaylandClient::QWaylandWindow* window
	) override;
};
