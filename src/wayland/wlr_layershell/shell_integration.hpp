#pragma once

#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <qtclasshelpermacros.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>

class QSWaylandLayerShellIntegration
    : public QtWaylandClient::QWaylandShellIntegrationTemplate<QSWaylandLayerShellIntegration>
    , public QtWayland::zwlr_layer_shell_v1 {
public:
	QSWaylandLayerShellIntegration();
	~QSWaylandLayerShellIntegration() override;
	Q_DISABLE_COPY_MOVE(QSWaylandLayerShellIntegration);

	QtWaylandClient::QWaylandShellSurface* createShellSurface(QtWaylandClient::QWaylandWindow* window
	) override;
};
