#include "surface.hpp"
#include <algorithm>
#include <any>

#include <private/qhighdpiscaling_p.h>
#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandscreen_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qrect.h>
#include <qsize.h>
#include <qtversionchecks.h>
#include <qtypes.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>

#include "../../window/panelinterface.hpp"
#include "shell_integration.hpp"
#include "window.hpp"

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#include <qpoint.h>
#endif

namespace {

[[nodiscard]] QtWayland::zwlr_layer_shell_v1::layer toWaylandLayer(const WlrLayer::Enum& layer
) noexcept {
	switch (layer) {
	case WlrLayer::Background: return QtWayland::zwlr_layer_shell_v1::layer_background;
	case WlrLayer::Bottom: return QtWayland::zwlr_layer_shell_v1::layer_bottom;
	case WlrLayer::Top: return QtWayland::zwlr_layer_shell_v1::layer_top;
	case WlrLayer::Overlay: return QtWayland::zwlr_layer_shell_v1::layer_overlay;
	}

	return QtWayland::zwlr_layer_shell_v1::layer_top;
}

[[nodiscard]] QtWayland::zwlr_layer_surface_v1::anchor toWaylandAnchors(const Anchors& anchors
) noexcept {
	quint32 wl = 0;
	if (anchors.mLeft) wl |= QtWayland::zwlr_layer_surface_v1::anchor_left;
	if (anchors.mRight) wl |= QtWayland::zwlr_layer_surface_v1::anchor_right;
	if (anchors.mTop) wl |= QtWayland::zwlr_layer_surface_v1::anchor_top;
	if (anchors.mBottom) wl |= QtWayland::zwlr_layer_surface_v1::anchor_bottom;
	return static_cast<QtWayland::zwlr_layer_surface_v1::anchor>(wl);
}

[[nodiscard]] QtWayland::zwlr_layer_surface_v1::keyboard_interactivity
toWaylandKeyboardFocus(const WlrKeyboardFocus::Enum& focus) noexcept {
	switch (focus) {
	case WlrKeyboardFocus::None: return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_none;
	case WlrKeyboardFocus::Exclusive:
		return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_exclusive;
	case WlrKeyboardFocus::OnDemand:
		return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_on_demand;
	}

	return QtWayland::zwlr_layer_surface_v1::keyboard_interactivity_none;
}

[[nodiscard]] QSize constrainedSize(const Anchors& anchors, const QSize& size) noexcept {
	return QSize(
	    anchors.horizontalConstraint() ? 0 : std::max(1, size.width()),
	    anchors.verticalConstraint() ? 0 : std::max(1, size.height())
	);
}

} // namespace

QSWaylandLayerSurface::QSWaylandLayerSurface(
    QSWaylandLayerShellIntegration* shell,
    QtWaylandClient::QWaylandWindow* window
)
    : QtWaylandClient::QWaylandShellSurface(window) {

	auto* qwindow = window->window();
	this->ext = LayershellWindowExtension::get(qwindow);

	if (this->ext == nullptr) {
		qFatal() << "QSWaylandLayerSurface created with null LayershellWindowExtension";
	}

	wl_output* output = nullptr; // NOLINT (include)
	if (this->ext->useWindowScreen) {
		auto* waylandScreen =
		    dynamic_cast<QtWaylandClient::QWaylandScreen*>(qwindow->screen()->handle());

		if (waylandScreen != nullptr) {
			output = waylandScreen->output();
		} else {
			qWarning(
			) << "Layershell screen does not corrospond to a real screen. Letting the compositor pick.";
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

	auto size = constrainedSize(this->ext->mAnchors, window->surfaceSize());
	this->set_size(size.width(), size.height());
}

QSWaylandLayerSurface::~QSWaylandLayerSurface() {
	if (this->ext != nullptr) {
		this->ext->surface = nullptr;
	}

	this->destroy();
}

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
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
		this->window()->handleExpose(QRect(QPoint(), this->size));
#elif QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
		this->window()->sendRecursiveExposeEvent();
#else
		this->window()->updateExposure();
#endif
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
	if (this->ext == nullptr) return;
	auto size = constrainedSize(this->ext->mAnchors, geometry.size());
	this->set_size(size.width(), size.height());
}

QWindow* QSWaylandLayerSurface::qwindow() { return this->window()->window(); }

void QSWaylandLayerSurface::updateLayer() {
	this->set_layer(toWaylandLayer(this->ext->mLayer));
	this->window()->waylandSurface()->commit();
}

void QSWaylandLayerSurface::updateAnchors() {
	this->set_anchor(toWaylandAnchors(this->ext->mAnchors));
	this->setWindowGeometry(this->window()->windowContentGeometry());
	this->window()->waylandSurface()->commit();
}

void QSWaylandLayerSurface::updateMargins() {
	auto& margins = this->ext->mMargins;
	this->set_margin(margins.mTop, margins.mRight, margins.mBottom, margins.mLeft);
	this->window()->waylandSurface()->commit();
}

void QSWaylandLayerSurface::updateExclusiveZone() {
	auto nativeZone = QHighDpi::toNativePixels(this->ext->mExclusiveZone, this->window()->window());
	this->set_exclusive_zone(nativeZone);
	this->window()->waylandSurface()->commit();
}

void QSWaylandLayerSurface::updateKeyboardFocus() {
	this->set_keyboard_interactivity(toWaylandKeyboardFocus(this->ext->mKeyboardFocus));
	this->window()->waylandSurface()->commit();
}

void QSWaylandLayerSurface::attachPopup(QtWaylandClient::QWaylandShellSurface* popup) {
	std::any role = popup->surfaceRole();

	if (auto* popupRole = std::any_cast<::xdg_popup*>(&role)) { // NOLINT
		this->get_popup(*popupRole);
	} else {
		qWarning() << "Cannot attach popup" << popup << "to shell surface" << this
		           << "as the popup is not an xdg_popup.";
	}
}
