#pragma once

#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qsize.h>
#include <qtclasshelpermacros.h>
#include <qtwaylandclientexports.h>
#include <qtypes.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>
#include <qwindow.h>

#include "shell_integration.hpp"
#include "wlr_layershell.hpp"

namespace qs::wayland::layershell {

struct LayerSurfaceState {
	QSize implicitSize;
	Anchors anchors;
	Margins margins;
	WlrLayer::Enum layer = WlrLayer::Top;
	qint32 exclusiveZone = 0;
	WlrKeyboardFocus::Enum keyboardFocus = WlrKeyboardFocus::None;

	bool compositorPickesScreen = true;
	QString mNamespace = "quickshell";

	[[nodiscard]] bool isCompatible(const LayerSurfaceState& other) const {
		return other.mNamespace == this->mNamespace;
	}
};

class LayerSurface;

class LayerSurfaceBridge: public QObject {
public:
	LayerSurfaceState state;

	void commitState();

	// Returns a bridge if attached, otherwise nullptr.
	static LayerSurfaceBridge* get(QWindow* window);

	// Creates or reuses a bridge on the given window and returns if it compatible, otherwise nullptr.
	static LayerSurfaceBridge* init(QWindow* window, LayerSurfaceState state);

private:
	explicit LayerSurfaceBridge(QWindow* parent): QObject(parent) {}

	LayerSurface* surface = nullptr;

	friend class LayerSurface;
};

class LayerSurface
    : public QtWaylandClient::QWaylandShellSurface
    , public QtWayland::zwlr_layer_surface_v1 {
public:
	LayerSurface(LayerShellIntegration* shell, QtWaylandClient::QWaylandWindow* window);

	~LayerSurface() override;
	Q_DISABLE_COPY_MOVE(LayerSurface);

	[[nodiscard]] bool isExposed() const override;
	void applyConfigure() override;
	void setWindowGeometry(const QRect& /*geometry*/) override {}

	void attachPopup(QtWaylandClient::QWaylandShellSurface* popup) override;

	void commit();

private:
	void zwlr_layer_surface_v1_configure(quint32 serial, quint32 width, quint32 height) override;
	void zwlr_layer_surface_v1_closed() override;

	QWindow* qwindow();

	LayerSurfaceBridge* bridge;
	bool configured = false;
	QSize size;

	LayerSurfaceState committed;
};

} // namespace qs::wayland::layershell
