#include "wlr_layershell.hpp"
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/panelinterface.hpp"
#include "../core/proxywindow.hpp"
#include "../core/qmlscreen.hpp"
#include "wlr_layershell/window.hpp"

WlrLayershell::WlrLayershell(QObject* parent)
    : ProxyWindowBase(parent)
    , ext(new LayershellWindowExtension(this)) {}

QQuickWindow* WlrLayershell::createWindow(QObject* oldInstance) {
	auto* old = qobject_cast<WlrLayershell*>(oldInstance);
	QQuickWindow* window = nullptr;

	if (old == nullptr || old->window == nullptr) {
		window = new QQuickWindow();
	} else {
		window = old->disownWindow();

		if (this->ext->attach(window)) {
			return window;
		} else {
			window->deleteLater();
			window = new QQuickWindow();
		}
	}

	if (!this->ext->attach(window)) {
		qWarning() << "Could not attach Layershell extension to new QQUickWindow. Layer will not "
		              "behave correctly.";
	}

	return window;
}

void WlrLayershell::setupWindow() {
	this->ProxyWindowBase::setupWindow();

	// clang-format off
	QObject::connect(this->ext, &LayershellWindowExtension::layerChanged, this, &WlrLayershell::layerChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::keyboardFocusChanged, this, &WlrLayershell::keyboardFocusChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::anchorsChanged, this, &WlrLayershell::anchorsChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::exclusiveZoneChanged, this, &WlrLayershell::exclusiveZoneChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::marginsChanged, this, &WlrLayershell::marginsChanged);

	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &WlrLayershell::updateAutoExclusion);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &WlrLayershell::updateAutoExclusion);
	QObject::connect(this, &WlrLayershell::anchorsChanged, this, &WlrLayershell::updateAutoExclusion);
	// clang-format on

	this->updateAutoExclusion();
}

void WlrLayershell::setWidth(qint32 width) {
	this->mWidth = width;

	// only update the actual size if not blocked by anchors
	if (!this->ext->anchors().horizontalConstraint()) {
		this->ProxyWindowBase::setWidth(width);
	}
}

void WlrLayershell::setHeight(qint32 height) {
	this->mHeight = height;

	// only update the actual size if not blocked by anchors
	if (!this->ext->anchors().verticalConstraint()) {
		this->ProxyWindowBase::setHeight(height);
	}
}

void WlrLayershell::setScreen(QuickshellScreenInfo* screen) {
	this->ProxyWindowBase::setScreen(screen);
	this->ext->setUseWindowScreen(screen != nullptr);
}

// NOLINTBEGIN
#define extPair(type, get, set)                                                                    \
	type WlrLayershell::get() const { return this->ext->get(); }                                     \
	void WlrLayershell::set(type value) { this->ext->set(value); }

extPair(WlrLayer::Enum, layer, setLayer);
extPair(WlrKeyboardFocus::Enum, keyboardFocus, setKeyboardFocus);
extPair(Margins, margins, setMargins);
// NOLINTEND

Anchors WlrLayershell::anchors() const { return this->ext->anchors(); }

void WlrLayershell::setAnchors(Anchors anchors) {
	this->ext->setAnchors(anchors);

	// explicitly set width values are tracked so the entire screen isn't covered if an anchor is removed.
	if (!anchors.horizontalConstraint()) this->ProxyWindowBase::setWidth(this->mWidth);
	if (!anchors.verticalConstraint()) this->ProxyWindowBase::setHeight(this->mHeight);
}

QString WlrLayershell::ns() const { return this->ext->ns(); }

void WlrLayershell::setNamespace(QString ns) {
	this->ext->setNamespace(std::move(ns));
	emit this->namespaceChanged();
}

qint32 WlrLayershell::exclusiveZone() const { return this->ext->exclusiveZone(); }

void WlrLayershell::setExclusiveZone(qint32 exclusiveZone) {
	this->mExclusiveZone = exclusiveZone;
	this->setExclusionMode(ExclusionMode::Normal);
	this->ext->setExclusiveZone(exclusiveZone);
}

ExclusionMode::Enum WlrLayershell::exclusionMode() const { return this->mExclusionMode; }

void WlrLayershell::setExclusionMode(ExclusionMode::Enum exclusionMode) {
	if (exclusionMode == this->mExclusionMode) return;
	this->mExclusionMode = exclusionMode;

	if (exclusionMode == ExclusionMode::Normal) {
		this->ext->setExclusiveZone(this->mExclusiveZone);
	} else if (exclusionMode == ExclusionMode::Ignore) {
		this->ext->setExclusiveZone(-1);
	} else {
		this->setAutoExclusion();
	}
}

void WlrLayershell::setAutoExclusion() {
	const auto anchors = this->anchors();
	auto zone = 0;

	if (anchors.horizontalConstraint()) zone = this->height();
	else if (anchors.verticalConstraint()) zone = this->width();

	this->ext->setExclusiveZone(zone);
}

void WlrLayershell::updateAutoExclusion() {
	if (this->mExclusionMode == ExclusionMode::Auto) {
		this->setAutoExclusion();
	}
}

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
	QObject::connect(this->layer, &ProxyWindowBase::heightChanged, this, &WaylandPanelInterface::heightChanged);
	QObject::connect(this->layer, &ProxyWindowBase::widthChanged, this, &WaylandPanelInterface::widthChanged);
	QObject::connect(this->layer, &ProxyWindowBase::screenChanged, this, &WaylandPanelInterface::screenChanged);
	QObject::connect(this->layer, &ProxyWindowBase::colorChanged, this, &WaylandPanelInterface::colorChanged);
	QObject::connect(this->layer, &ProxyWindowBase::maskChanged, this, &WaylandPanelInterface::maskChanged);

	// panel specific
	QObject::connect(this->layer, &WlrLayershell::anchorsChanged, this, &WaylandPanelInterface::anchorsChanged);
	QObject::connect(this->layer, &WlrLayershell::marginsChanged, this, &WaylandPanelInterface::marginsChanged);
	QObject::connect(this->layer, &WlrLayershell::exclusiveZoneChanged, this, &WaylandPanelInterface::exclusiveZoneChanged);
	QObject::connect(this->layer, &WlrLayershell::exclusionModeChanged, this, &WaylandPanelInterface::exclusionModeChanged);
	// clang-format on
}

void WaylandPanelInterface::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<WaylandPanelInterface*>(oldInstance);
	this->layer->onReload(old != nullptr ? old->layer : nullptr);
}

QQmlListProperty<QObject> WaylandPanelInterface::data() { return this->layer->data(); }
ProxyWindowBase* WaylandPanelInterface::proxyWindow() const { return this->layer; }
QQuickItem* WaylandPanelInterface::contentItem() const { return this->layer->contentItem(); }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type WaylandPanelInterface::get() const { return this->layer->get(); }                           \
	void WaylandPanelInterface::set(type value) { this->layer->set(value); }

proxyPair(bool, isVisible, setVisible);
proxyPair(qint32, width, setWidth);
proxyPair(qint32, height, setHeight);
proxyPair(QuickshellScreenInfo*, screen, setScreen);
proxyPair(QColor, color, setColor);
proxyPair(PendingRegion*, mask, setMask);

// panel specific
proxyPair(Anchors, anchors, setAnchors);
proxyPair(Margins, margins, setMargins);
proxyPair(qint32, exclusiveZone, setExclusiveZone);
proxyPair(ExclusionMode::Enum, exclusionMode, setExclusionMode);

#undef proxyPair
// NOLINTEND
