#include "waylandlayershell.hpp"
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
#include "layershell.hpp"

WaylandLayershell::WaylandLayershell(QObject* parent)
    : ProxyWindowBase(parent)
    , ext(new LayershellWindowExtension(this)) {}

QQuickWindow* WaylandLayershell::createWindow(QObject* oldInstance) {
	auto* old = qobject_cast<WaylandLayershell*>(oldInstance);
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

void WaylandLayershell::setupWindow() {
	this->ProxyWindowBase::setupWindow();

	// clang-format off
	QObject::connect(this->ext, &LayershellWindowExtension::layerChanged, this, &WaylandLayershell::layerChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::keyboardFocusChanged, this, &WaylandLayershell::keyboardFocusChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::anchorsChanged, this, &WaylandLayershell::anchorsChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::exclusiveZoneChanged, this, &WaylandLayershell::exclusiveZoneChanged);
	QObject::connect(this->ext, &LayershellWindowExtension::marginsChanged, this, &WaylandLayershell::marginsChanged);

	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &WaylandLayershell::updateAutoExclusion);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &WaylandLayershell::updateAutoExclusion);
	QObject::connect(this, &WaylandLayershell::anchorsChanged, this, &WaylandLayershell::updateAutoExclusion);
	QObject::connect(this, &WaylandLayershell::marginsChanged, this, &WaylandLayershell::updateAutoExclusion);
	// clang-format on
}

void WaylandLayershell::setWidth(qint32 width) {
	this->mWidth = width;

	// only update the actual size if not blocked by anchors
	if (!this->ext->anchors().horizontalConstraint()) {
		this->ProxyWindowBase::setWidth(width);
	}
}

void WaylandLayershell::setHeight(qint32 height) {
	this->mHeight = height;

	// only update the actual size if not blocked by anchors
	if (!this->ext->anchors().verticalConstraint()) {
		this->ProxyWindowBase::setHeight(height);
	}
}

void WaylandLayershell::setScreen(QuickShellScreenInfo* screen) {
	this->ProxyWindowBase::setScreen(screen);
	this->ext->setUseWindowScreen(screen != nullptr);
}

// NOLINTBEGIN
#define extPair(type, get, set)                                                                    \
	type WaylandLayershell::get() const { return this->ext->get(); }                                 \
	void WaylandLayershell::set(type value) { this->ext->set(value); }

extPair(Layer::Enum, layer, setLayer);
extPair(KeyboardFocus::Enum, keyboardFocus, setKeyboardFocus);
extPair(Margins, margins, setMargins);
// NOLINTEND

Anchors WaylandLayershell::anchors() const { return this->ext->anchors(); }

void WaylandLayershell::setAnchors(Anchors anchors) {
	this->ext->setAnchors(anchors);

	// explicitly set width values are tracked so the entire screen isn't covered if an anchor is removed.
	if (!anchors.horizontalConstraint()) this->ProxyWindowBase::setWidth(this->mWidth);
	if (!anchors.verticalConstraint()) this->ProxyWindowBase::setHeight(this->mHeight);
}

QString WaylandLayershell::ns() const { return this->ext->ns(); }

void WaylandLayershell::setNamespace(QString ns) {
	this->ext->setNamespace(std::move(ns));
	emit this->namespaceChanged();
}

qint32 WaylandLayershell::exclusiveZone() const { return this->ext->exclusiveZone(); }

void WaylandLayershell::setExclusiveZone(qint32 exclusiveZone) {
	qDebug() << "set exclusion" << exclusiveZone;
	this->mExclusiveZone = exclusiveZone;

	if (this->mExclusionMode == ExclusionMode::Normal) {
		this->ext->setExclusiveZone(exclusiveZone);
	}
}

ExclusionMode::Enum WaylandLayershell::exclusionMode() const { return this->mExclusionMode; }

void WaylandLayershell::setExclusionMode(ExclusionMode::Enum exclusionMode) {
	this->mExclusionMode = exclusionMode;
	if (exclusionMode == ExclusionMode::Normal) {
		this->ext->setExclusiveZone(this->mExclusiveZone);
	} else if (exclusionMode == ExclusionMode::Ignore) {
		this->ext->setExclusiveZone(-1);
	} else {
		this->setAutoExclusion();
	}
}

void WaylandLayershell::setAutoExclusion() {
	const auto anchors = this->anchors();
	auto zone = 0;

	if (anchors.horizontalConstraint()) zone = this->height();
	else if (anchors.verticalConstraint()) zone = this->width();

	this->ext->setExclusiveZone(zone);
}

void WaylandLayershell::updateAutoExclusion() {
	if (this->mExclusionMode == ExclusionMode::Auto) {
		this->setAutoExclusion();
	}
}

// WaylandPanelInterface

WaylandPanelInterface::WaylandPanelInterface(QObject* parent)
    : PanelWindowInterface(parent)
    , layer(new WaylandLayershell(this)) {

	// clang-format off
	QObject::connect(this->layer, &ProxyWindowBase::windowConnected, this, &WaylandPanelInterface::windowConnected);
	QObject::connect(this->layer, &ProxyWindowBase::visibleChanged, this, &WaylandPanelInterface::visibleChanged);
	QObject::connect(this->layer, &ProxyWindowBase::heightChanged, this, &WaylandPanelInterface::heightChanged);
	QObject::connect(this->layer, &ProxyWindowBase::widthChanged, this, &WaylandPanelInterface::widthChanged);
	QObject::connect(this->layer, &ProxyWindowBase::screenChanged, this, &WaylandPanelInterface::screenChanged);
	QObject::connect(this->layer, &ProxyWindowBase::colorChanged, this, &WaylandPanelInterface::colorChanged);
	QObject::connect(this->layer, &ProxyWindowBase::maskChanged, this, &WaylandPanelInterface::maskChanged);

	// panel specific
	QObject::connect(this->layer, &WaylandLayershell::anchorsChanged, this, &WaylandPanelInterface::anchorsChanged);
	QObject::connect(this->layer, &WaylandLayershell::marginsChanged, this, &WaylandPanelInterface::marginsChanged);
	QObject::connect(this->layer, &WaylandLayershell::exclusiveZoneChanged, this, &WaylandPanelInterface::exclusiveZoneChanged);
	QObject::connect(this->layer, &WaylandLayershell::exclusionModeChanged, this, &WaylandPanelInterface::exclusionModeChanged);
	// clang-format on
}

void WaylandPanelInterface::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<WaylandPanelInterface*>(oldInstance);
	this->layer->onReload(old != nullptr ? old->layer : nullptr);
}

QQmlListProperty<QObject> WaylandPanelInterface::data() { return this->layer->data(); }
QQuickItem* WaylandPanelInterface::contentItem() const { return this->layer->contentItem(); }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type WaylandPanelInterface::get() const { return this->layer->get(); }                           \
	void WaylandPanelInterface::set(type value) { this->layer->set(value); }

proxyPair(bool, isVisible, setVisible);
proxyPair(qint32, width, setWidth);
proxyPair(qint32, height, setHeight);
proxyPair(QuickShellScreenInfo*, screen, setScreen);
proxyPair(QColor, color, setColor);
proxyPair(PendingRegion*, mask, setMask);

// panel specific
proxyPair(Anchors, anchors, setAnchors);
proxyPair(Margins, margins, setMargins);
proxyPair(qint32, exclusiveZone, setExclusiveZone);
proxyPair(ExclusionMode::Enum, exclusionMode, setExclusionMode);
// NOLINTEND
