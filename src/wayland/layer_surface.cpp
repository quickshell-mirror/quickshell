#include "layer_surface.hpp"

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandscreen_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qtypes.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>

#include "../core/shellwindow.hpp"
#include "layershell.hpp"
#include "shell_integration.hpp"

// clang-format off
[[nodiscard]] QtWayland::zwlr_layer_shell_v1::layer toWaylandLayer(const Layer::Enum& layer) noexcept;
[[nodiscard]] QtWayland::zwlr_layer_surface_v1::anchor toWaylandAnchors(const Anchors& anchors) noexcept;
[[nodiscard]] QtWayland::zwlr_layer_surface_v1::keyboard_interactivity toWaylandKeyboardFocus(const KeyboardFocus::Enum& focus) noexcept;
[[nodiscard]] QSize constrainedSize(const Anchors& anchors, const QSize& size) noexcept;
// clang-format on

// clang-format off
QSWaylandLayerSurface::QSWaylandLayerSurface(
    QSWaylandLayerShellIntegration* shell,
    QtWaylandClient::QWaylandWindow* window
): QtWaylandClient::QWaylandShellSurface(window) {
	// clang-format on

	auto* qwindow = window->window();
	this->ext = LayershellWindowExtension::get(qwindow);

	if (this->ext == nullptr) {
		throw "QSWaylandLayerSurface created with null LayershellWindowExtension";
	}

	wl_output* output = nullptr; // NOLINT (import)
	if (this->ext->useWindowScreen) {
		auto* waylandScreen =
		    dynamic_cast<QtWaylandClient::QWaylandScreen*>(qwindow->screen()->handle());

		if (waylandScreen != nullptr) {
			output = waylandScreen->output();
		} else {
			qWarning() << "Layershell screen is set but does not corrospond to a real screen. Letting "
			              "the compositor pick.";
		}
	}

	this->init(shell->get_layer_surface(
	    window->waylandSurface()->object(),
	    output,
	    toWaylandLayer(this->ext->mLayer),
	    this->ext->mNamespace
	));

	this->updateAnchors();
	this->updateLayer();
	this->updateMargins();
	this->updateExclusiveZone();
	this->updateKeyboardFocus();

	// new updates will be sent from the extension
	this->ext->surface = this;

	auto size = constrainedSize(this->ext->mAnchors, qwindow->size());
	this->set_size(size.width(), size.height());
}

QSWaylandLayerSurface::~QSWaylandLayerSurface() { this->destroy(); }

void QSWaylandLayerSurface::zwlr_layer_surface_v1_configure(
    quint32 serial,
    quint32 width,
    quint32 height
) {
	this->ack_configure(serial);

	this->size = QSize(static_cast<qint32>(width), static_cast<qint32>(height));
	if (!this->configured) {
		this->configured = true;
		this->window()->resizeFromApplyConfigure(this->size);
		this->window()->handleExpose(QRect(QPoint(), this->size));
	} else {
		this->window()->applyConfigureWhenPossible();
	}
}

void QSWaylandLayerSurface::zwlr_layer_surface_v1_closed() { this->window()->window()->close(); }

bool QSWaylandLayerSurface::isExposed() const { return this->configured; }

void QSWaylandLayerSurface::applyConfigure() {
	this->window()->resizeFromApplyConfigure(this->size);
}

void QSWaylandLayerSurface::setWindowGeometry(const QRect& geometry) {
	auto size = constrainedSize(this->ext->mAnchors, geometry.size());
	this->set_size(size.width(), size.height());
}

QWindow* QSWaylandLayerSurface::qwindow() { return this->window()->window(); }

void QSWaylandLayerSurface::updateLayer() { this->set_layer(toWaylandLayer(this->ext->mLayer)); }

void QSWaylandLayerSurface::updateAnchors() {
	this->set_anchor(toWaylandAnchors(this->ext->mAnchors));
	this->setWindowGeometry(this->window()->windowContentGeometry());
}

void QSWaylandLayerSurface::updateMargins() {
	auto& margins = this->ext->mMargins;
	this->set_margin(margins.mTop, margins.mRight, margins.mBottom, margins.mLeft);
}

void QSWaylandLayerSurface::updateExclusiveZone() {
	this->set_exclusive_zone(this->ext->mExclusiveZone);
}

void QSWaylandLayerSurface::updateKeyboardFocus() {
	this->set_keyboard_interactivity(toWaylandKeyboardFocus(this->ext->mKeyboardFocus));
}

QtWayland::zwlr_layer_shell_v1::layer toWaylandLayer(const Layer::Enum& layer) noexcept {
	switch (layer) {
	case Layer::Background: return QtWayland::zwlr_layer_shell_v1::layer_background;
	case Layer::Bottom: return QtWayland::zwlr_layer_shell_v1::layer_bottom;
	case Layer::Top: return QtWayland::zwlr_layer_shell_v1::layer_top;
	case Layer::Overlay: return QtWayland::zwlr_layer_shell_v1::layer_overlay;
	}

	return QtWayland::zwlr_layer_shell_v1::layer_top;
}

QtWayland::zwlr_layer_surface_v1::anchor toWaylandAnchors(const Anchors& anchors) noexcept {
	quint32 wl = 0;
	if (anchors.mLeft) wl |= QtWayland::zwlr_layer_surface_v1::anchor_left;
	if (anchors.mRight) wl |= QtWayland::zwlr_layer_surface_v1::anchor_right;
	if (anchors.mTop) wl |= QtWayland::zwlr_layer_surface_v1::anchor_top;
	if (anchors.mBottom) wl |= QtWayland::zwlr_layer_surface_v1::anchor_bottom;
	return static_cast<QtWayland::zwlr_layer_surface_v1::anchor>(wl);
}

QtWayland::zwlr_layer_surface_v1::keyboard_interactivity
toWaylandKeyboardFocus(const KeyboardFocus::Enum& focus) noexcept {
	switch (focus) {
	case KeyboardFocus::None: return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_none;
	case KeyboardFocus::Exclusive:
		return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_exclusive;
	case KeyboardFocus::OnDemand:
		return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_on_demand;
	}

	return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_none;
}

QSize constrainedSize(const Anchors& anchors, const QSize& size) noexcept {
	return QSize(
	    anchors.horizontalConstraint() ? 0 : size.width(),
	    anchors.verticalConstraint() ? 0 : size.height()
	);
}
