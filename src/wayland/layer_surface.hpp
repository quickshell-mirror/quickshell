#pragma once

#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <qtwaylandclientexports.h>
#include <qtypes.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>
#include <qwindow.h>

#include "layershell.hpp"
#include "shell_integration.hpp"

class Q_WAYLANDCLIENT_EXPORT QSWaylandLayerSurface
    : public QtWaylandClient::QWaylandShellSurface
    , public QtWayland::zwlr_layer_surface_v1 {
public:
	QSWaylandLayerSurface(
	    QSWaylandLayerShellIntegration* shell,
	    QtWaylandClient::QWaylandWindow* window
	);

	~QSWaylandLayerSurface() override;
	QSWaylandLayerSurface(QSWaylandLayerSurface&&) = delete;
	QSWaylandLayerSurface(const QSWaylandLayerSurface&) = delete;
	void operator=(QSWaylandLayerSurface&&) = delete;
	void operator=(const QSWaylandLayerSurface&) = delete;

	[[nodiscard]] bool isExposed() const override;
	void applyConfigure() override;
	void setWindowGeometry(const QRect& geometry) override;

private:
	void zwlr_layer_surface_v1_configure(quint32 serial, quint32 width, quint32 height) override;
	void zwlr_layer_surface_v1_closed() override;

	QWindow* qwindow();
	void updateLayer();
	void updateAnchors();
	void updateMargins();
	void updateExclusiveZone();
	void updateKeyboardFocus();

	LayershellWindowExtension* ext;
	QSize size;
	bool configured = false;

	friend class LayershellWindowExtension;
};
