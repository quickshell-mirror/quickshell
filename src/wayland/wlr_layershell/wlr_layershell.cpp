#include "wlr_layershell.hpp"

#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtypes.h>

#include "../../core/qmlscreen.hpp"
#include "../../window/panelinterface.hpp"
#include "../../window/proxywindow.hpp"
#include "surface.hpp"

namespace qs::wayland::layershell {

WlrLayershell::WlrLayershell(QObject* parent): ProxyWindowBase(parent) {
	this->bcExclusiveZone.setBinding([this]() -> qint32 {
		switch (this->bExclusionMode.value()) {
		case ExclusionMode::Ignore: return -1;
		case ExclusionMode::Normal: return this->bExclusiveZone;
		case ExclusionMode::Auto:
			const auto margins = this->bMargins.value();

			// add reverse edge margins which are ignored by wlr-layer-shell
			switch (this->bcExclusionEdge.value()) {
			case Qt::TopEdge: return this->bImplicitHeight + margins.bottom;
			case Qt::BottomEdge: return this->bImplicitHeight + margins.top;
			case Qt::LeftEdge: return this->bImplicitHeight + margins.right;
			case Qt::RightEdge: return this->bImplicitHeight + margins.left;
			default: return 0;
			}
		}
	});

	this->bcExclusionEdge.setBinding([this] { return this->bAnchors.value().exclusionEdge(); });
}

ProxiedWindow* WlrLayershell::retrieveWindow(QObject* oldInstance) {
	auto* old = qobject_cast<WlrLayershell*>(oldInstance);
	auto* window = old == nullptr ? nullptr : old->disownWindow();

	if (window != nullptr) {
		this->connectBridge(LayerSurfaceBridge::init(window, this->computeState()));

		if (this->bridge) {
			return window;
		} else {
			window->deleteLater();
		}
	}

	return this->createQQuickWindow();
}

ProxiedWindow* WlrLayershell::createQQuickWindow() {
	auto* window = this->ProxyWindowBase::createQQuickWindow();

	this->connectBridge(LayerSurfaceBridge::init(window, this->computeState()));
	if (!this->bridge) {
		qWarning() << "Could not attach Layershell extension to new QQuickWindow. Layer will not "
		              "behave correctly.";
	}

	return window;
}

void WlrLayershell::connectWindow() {
	this->ProxyWindowBase::connectWindow();

	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &WlrLayershell::updateAutoExclusion);

	QObject::connect(
	    this,
	    &ProxyWindowBase::heightChanged,
	    this,
	    &WlrLayershell::updateAutoExclusion
	);

	this->updateAutoExclusion();
}

ProxiedWindow* WlrLayershell::disownWindow(bool keepItemOwnership) {
	auto* window = this->ProxyWindowBase::disownWindow(keepItemOwnership);

	if (this->bridge) {
		this->connectBridge(nullptr);
	}

	return window;
}

void WlrLayershell::connectBridge(LayerSurfaceBridge* bridge) {
	if (this->bridge) {
		QObject::disconnect(this->bridge, nullptr, this, nullptr);
	}

	this->bridge = bridge;

	if (bridge) {
		QObject::connect(this->bridge, &QObject::destroyed, this, &WlrLayershell::onBridgeDestroyed);
	}
}

void WlrLayershell::onBridgeDestroyed() { this->bridge = nullptr; }

bool WlrLayershell::deleteOnInvisible() const {
	// Qt windows behave weirdly when geometry is modified and setVisible(false)
	// is subsequently called in the same frame.
	// It will attach buffers to the wayland surface unconditionally before
	// the surface recieves a configure event, causing a protocol error.
	// To remedy this we forcibly disallow window reuse.
	return true;
}

void WlrLayershell::onPolished() {
	if (this->bridge) {
		this->bridge->state = this->computeState();
		this->bridge->commitState();
	}

	this->ProxyWindowBase::onPolished();
}

void WlrLayershell::trySetWidth(qint32 /*implicitWidth*/) { this->onStateChanged(); }
void WlrLayershell::trySetHeight(qint32 /*implicitHeight*/) { this->onStateChanged(); }

void WlrLayershell::setScreen(QuickshellScreenInfo* screen) {
	this->compositorPicksScreen = screen == nullptr;
	this->ProxyWindowBase::setScreen(screen);
}

void WlrLayershell::onStateChanged() { this->schedulePolish(); }

bool WlrLayershell::aboveWindows() const { return this->layer() > WlrLayer::Bottom; }

void WlrLayershell::setAboveWindows(bool aboveWindows) {
	this->setLayer(aboveWindows ? WlrLayer::Top : WlrLayer::Bottom);
}

bool WlrLayershell::focusable() const { return this->keyboardFocus() != WlrKeyboardFocus::None; }

void WlrLayershell::setFocusable(bool focusable) {
	this->setKeyboardFocus(focusable ? WlrKeyboardFocus::OnDemand : WlrKeyboardFocus::None);
}

LayerSurfaceState WlrLayershell::computeState() const {
	return LayerSurfaceState {
	    .implicitSize = QSize(this->implicitWidth(), this->implicitHeight()),
	    .anchors = this->bAnchors,
	    .margins = this->bMargins,
	    .layer = this->bLayer,
	    .exclusiveZone = this->bcExclusiveZone,
	    .keyboardFocus = this->bKeyboardFocus,
	    .compositorPickesScreen = this->compositorPicksScreen,
	    .mNamespace = this->bNamespace,
	};
}

void WlrLayershell::updateAutoExclusion() { this->bcExclusiveZone.notify(); }

WlrLayershell* WlrLayershell::qmlAttachedProperties(QObject* object) {
	if (auto* obj = qobject_cast<WaylandPanelInterface*>(object)) {
		return obj->layer;
	} else {
		return nullptr;
	}
}

// WaylandPanelInterface

WaylandPanelInterface::WaylandPanelInterface(QObject* parent)
    : PanelWindowInterface(parent)
    , layer(new WlrLayershell(this)) {

	// clang-format off
	QObject::connect(this->layer, &ProxyWindowBase::windowConnected, this, &WaylandPanelInterface::windowConnected);
	QObject::connect(this->layer, &ProxyWindowBase::visibleChanged, this, &WaylandPanelInterface::visibleChanged);
	QObject::connect(this->layer, &ProxyWindowBase::backerVisibilityChanged, this, &WaylandPanelInterface::backingWindowVisibleChanged);
	QObject::connect(this->layer, &ProxyWindowBase::implicitHeightChanged, this, &WaylandPanelInterface::implicitHeightChanged);
	QObject::connect(this->layer, &ProxyWindowBase::implicitWidthChanged, this, &WaylandPanelInterface::implicitWidthChanged);
	QObject::connect(this->layer, &ProxyWindowBase::heightChanged, this, &WaylandPanelInterface::heightChanged);
	QObject::connect(this->layer, &ProxyWindowBase::widthChanged, this, &WaylandPanelInterface::widthChanged);
	QObject::connect(this->layer, &ProxyWindowBase::devicePixelRatioChanged, this, &WaylandPanelInterface::devicePixelRatioChanged);
	QObject::connect(this->layer, &ProxyWindowBase::screenChanged, this, &WaylandPanelInterface::screenChanged);
	QObject::connect(this->layer, &ProxyWindowBase::windowTransformChanged, this, &WaylandPanelInterface::windowTransformChanged);
	QObject::connect(this->layer, &ProxyWindowBase::colorChanged, this, &WaylandPanelInterface::colorChanged);
	QObject::connect(this->layer, &ProxyWindowBase::maskChanged, this, &WaylandPanelInterface::maskChanged);
	QObject::connect(this->layer, &ProxyWindowBase::surfaceFormatChanged, this, &WaylandPanelInterface::surfaceFormatChanged);

	// panel specific
	QObject::connect(this->layer, &WlrLayershell::anchorsChanged, this, &WaylandPanelInterface::anchorsChanged);
	QObject::connect(this->layer, &WlrLayershell::marginsChanged, this, &WaylandPanelInterface::marginsChanged);
	QObject::connect(this->layer, &WlrLayershell::exclusiveZoneChanged, this, &WaylandPanelInterface::exclusiveZoneChanged);
	QObject::connect(this->layer, &WlrLayershell::exclusionModeChanged, this, &WaylandPanelInterface::exclusionModeChanged);
	QObject::connect(this->layer, &WlrLayershell::layerChanged, this, &WaylandPanelInterface::aboveWindowsChanged);
	QObject::connect(this->layer, &WlrLayershell::keyboardFocusChanged, this, &WaylandPanelInterface::focusableChanged);
	// clang-format on
}

void WaylandPanelInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->layer, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<WaylandPanelInterface*>(oldInstance);
	this->layer->reload(old != nullptr ? old->layer : nullptr);
}

QQmlListProperty<QObject> WaylandPanelInterface::data() { return this->layer->data(); }
ProxyWindowBase* WaylandPanelInterface::proxyWindow() const { return this->layer; }
QQuickItem* WaylandPanelInterface::contentItem() const { return this->layer->contentItem(); }

bool WaylandPanelInterface::isBackingWindowVisible() const {
	return this->layer->isVisibleDirect();
}

qreal WaylandPanelInterface::devicePixelRatio() const { return this->layer->devicePixelRatio(); }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type WaylandPanelInterface::get() const { return this->layer->get(); }                           \
	void WaylandPanelInterface::set(type value) { this->layer->set(value); }

proxyPair(bool, isVisible, setVisible);
proxyPair(qint32, implicitWidth, setImplicitWidth);
proxyPair(qint32, implicitHeight, setImplicitHeight);
proxyPair(qint32, width, setWidth);
proxyPair(qint32, height, setHeight);
proxyPair(QuickshellScreenInfo*, screen, setScreen);
proxyPair(QColor, color, setColor);
proxyPair(PendingRegion*, mask, setMask);
proxyPair(QsSurfaceFormat, surfaceFormat, setSurfaceFormat);

// panel specific
proxyPair(Anchors, anchors, setAnchors);
proxyPair(Margins, margins, setMargins);
proxyPair(qint32, exclusiveZone, setExclusiveZone);
proxyPair(ExclusionMode::Enum, exclusionMode, setExclusionMode);
proxyPair(bool, focusable, setFocusable);
proxyPair(bool, aboveWindows, setAboveWindows);

#undef proxyPair
// NOLINTEND

} // namespace qs::wayland::layershell
