#pragma once

#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <qtclasshelpermacros.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>

namespace qs::wayland::layershell {

class LayerShellIntegration
    : public QtWaylandClient::QWaylandShellIntegrationTemplate<LayerShellIntegration>
    , public QtWayland::zwlr_layer_shell_v1 {
public:
	LayerShellIntegration();
	~LayerShellIntegration() override;
	Q_DISABLE_COPY_MOVE(LayerShellIntegration);

	QtWaylandClient::QWaylandShellSurface* createShellSurface(QtWaylandClient::QWaylandWindow* window
	) override;
};

} // namespace qs::wayland::layershell
