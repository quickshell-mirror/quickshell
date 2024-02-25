#pragma once

#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <qtwaylandclientexports.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>

class QSWaylandLayerShellIntegration
    : public QtWaylandClient::QWaylandShellIntegrationTemplate<QSWaylandLayerShellIntegration>
    , public QtWayland::zwlr_layer_shell_v1 {
public:
	QSWaylandLayerShellIntegration();
	~QSWaylandLayerShellIntegration() override;
	QSWaylandLayerShellIntegration(QSWaylandLayerShellIntegration&&) = delete;
	QSWaylandLayerShellIntegration(const QSWaylandLayerShellIntegration&) = delete;
	void operator=(QSWaylandLayerShellIntegration&&) = delete;
	void operator=(const QSWaylandLayerShellIntegration&) = delete;

	QtWaylandClient::QWaylandShellSurface* createShellSurface(QtWaylandClient::QWaylandWindow* window
	) override;
};
