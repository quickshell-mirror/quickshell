#include "surface.hpp"
#include <algorithm>
#include <any>
#include <utility>

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
#include <qvariant.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>
#include <qwindow.h>

#include "../../window/panelinterface.hpp"
#include "shell_integration.hpp"
#include "wlr_layershell.hpp"

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#include <qpoint.h>
#endif

namespace qs::wayland::layershell {

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

void LayerSurfaceBridge::commitState() {
	if (this->surface) this->surface->commit();
}

LayerSurfaceBridge* LayerSurfaceBridge::get(QWindow* window) {
	auto v = window->property("layershell_bridge");

	if (v.canConvert<LayerSurfaceBridge*>()) {
		return v.value<LayerSurfaceBridge*>();
	}

	return nullptr;
}

LayerSurfaceBridge* LayerSurfaceBridge::init(QWindow* window, LayerSurfaceState state) {
	auto* bridge = LayerSurfaceBridge::get(window);

	if (!bridge) {
		bridge = new LayerSurfaceBridge(window);
		window->setProperty("layershell_bridge", QVariant::fromValue(bridge));
	} else if (!bridge->state.isCompatible(state)) {
		return nullptr;
	}

	if (!bridge->surface) {
		// Qt appears to be resetting the window's screen on creation on some systems. This works around it.
		auto* screen = window->screen();
		window->create();
		window->setScreen(screen);

		auto* waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
		if (waylandWindow == nullptr) {
			qWarning() << window << "is not a wayland window. Cannot create layershell surface.";
			return nullptr;
		}

		static LayerShellIntegration* layershellIntegration = nullptr; // NOLINT
		if (layershellIntegration == nullptr) {
			layershellIntegration = new LayerShellIntegration();
			if (!layershellIntegration->initialize(waylandWindow->display())) {
				delete layershellIntegration;
				layershellIntegration = nullptr;
				qWarning() << "Failed to initialize layershell integration";
			}
		}

		waylandWindow->setShellIntegration(layershellIntegration);
	}

	bridge->state = std::move(state);
	bridge->commitState();

	return bridge;
}

LayerSurface::LayerSurface(LayerShellIntegration* shell, QtWaylandClient::QWaylandWindow* window)
    : QtWaylandClient::QWaylandShellSurface(window) {

	auto* qwindow = window->window();

	this->bridge = LayerSurfaceBridge::get(qwindow);
	if (this->bridge == nullptr) qFatal() << "LayerSurface created with null bridge";
	const auto& s = this->bridge->state;

	wl_output* output = nullptr; // NOLINT (include)
	if (!s.compositorPickesScreen) {
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
	    toWaylandLayer(s.layer),
	    s.mNamespace
	));

	auto size = constrainedSize(s.anchors, QHighDpi::toNativePixels(s.implicitSize, qwindow));
	this->set_size(size.width(), size.height());
	this->set_anchor(toWaylandAnchors(s.anchors));
	this->set_margin(
	    QHighDpi::toNativePixels(s.margins.top, qwindow),
	    QHighDpi::toNativePixels(s.margins.right, qwindow),
	    QHighDpi::toNativePixels(s.margins.bottom, qwindow),
	    QHighDpi::toNativePixels(s.margins.left, qwindow)
	);
	this->set_exclusive_zone(QHighDpi::toNativePixels(s.exclusiveZone, qwindow));
	this->set_keyboard_interactivity(toWaylandKeyboardFocus(s.keyboardFocus));

	this->bridge->surface = this;
}

LayerSurface::~LayerSurface() {
	if (this->bridge && this->bridge->surface == this) {
		this->bridge->surface = nullptr;
	}

	this->destroy();
}

void LayerSurface::zwlr_layer_surface_v1_configure(quint32 serial, quint32 width, quint32 height) {
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

void LayerSurface::zwlr_layer_surface_v1_closed() { this->window()->window()->close(); }

bool LayerSurface::isExposed() const { return this->configured; }

void LayerSurface::applyConfigure() { this->window()->resizeFromApplyConfigure(this->size); }

QWindow* LayerSurface::qwindow() { return this->window()->window(); }

void LayerSurface::commit() {
	const auto& p = this->bridge->state;
	auto& c = this->committed;

	if (p.implicitSize != c.implicitSize || p.anchors != c.anchors) {
		auto size =
		    constrainedSize(p.anchors, QHighDpi::toNativePixels(p.implicitSize, this->qwindow()));
		this->set_size(size.width(), size.height());
	}

	if (p.anchors != c.anchors) {
		this->set_anchor(toWaylandAnchors(p.anchors));
	}

	if (p.margins != c.margins) {
		this->set_margin(
		    QHighDpi::toNativePixels(p.margins.top, this->qwindow()),
		    QHighDpi::toNativePixels(p.margins.right, this->qwindow()),
		    QHighDpi::toNativePixels(p.margins.bottom, this->qwindow()),
		    QHighDpi::toNativePixels(p.margins.left, this->qwindow())
		);
	}

	if (p.layer != c.layer) {
		this->set_layer(p.layer);
	}

	if (p.exclusiveZone != c.exclusiveZone) {
		this->set_exclusive_zone(QHighDpi::toNativePixels(p.exclusiveZone, this->qwindow()));
	}

	if (p.keyboardFocus != c.keyboardFocus) {
		this->set_keyboard_interactivity(toWaylandKeyboardFocus(p.keyboardFocus));
	}

	c = p;
}

void LayerSurface::attachPopup(QtWaylandClient::QWaylandShellSurface* popup) {
	std::any role = popup->surfaceRole();

	if (auto* popupRole = std::any_cast<::xdg_popup*>(&role)) { // NOLINT
		this->get_popup(*popupRole);
	} else {
		qWarning() << "Cannot attach popup" << popup << "to shell surface" << this
		           << "as the popup is not an xdg_popup.";
	}
}

} // namespace qs::wayland::layershell
