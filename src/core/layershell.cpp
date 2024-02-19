#include "layershell.hpp"

#include <LayerShellQt/window.h>
#include <qmargins.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "proxywindow.hpp"
#include "shellwindow.hpp"

WaylandShellWindow::WaylandShellWindow(QObject* parent):
    ProxyShellWindow(parent), mWayland(new WaylandShellWindowExtensions(this)) {}

void WaylandShellWindow::setupWindow() {
	this->shellWindow = LayerShellQt::Window::get(this->window);

	this->ProxyShellWindow::setupWindow();

	// clang-format off
	QObject::connect(this->shellWindow, &LayerShellQt::Window::anchorsChanged, this, &ProxyShellWindow::anchorsChanged);
	QObject::connect(this->shellWindow, &LayerShellQt::Window::marginsChanged, this, &ProxyShellWindow::marginsChanged);

	QObject::connect(
	    this->shellWindow, &LayerShellQt::Window::layerChanged,
	    this->mWayland, &WaylandShellWindowExtensions::layerChanged
	 );
	QObject::connect(
	    this->shellWindow, &LayerShellQt::Window::keyboardInteractivityChanged,
	    this->mWayland, &WaylandShellWindowExtensions::keyboardFocusChanged
	);

	QObject::connect(this->window, &QWindow::widthChanged, this, &WaylandShellWindow::updateExclusionZone);
	QObject::connect(this->window, &QWindow::heightChanged, this, &WaylandShellWindow::updateExclusionZone);
	QObject::connect(this, &ProxyShellWindow::anchorsChanged, this, &WaylandShellWindow::updateExclusionZone);
	QObject::connect(this, &ProxyShellWindow::marginsChanged, this, &WaylandShellWindow::updateExclusionZone);
	// clang-format on

	this->setAnchors(this->mAnchors);
	this->setMargins(this->mMargins);
	this->setExclusionMode(this->mExclusionMode); // also sets exclusion zone
	this->mWayland->setLayer(this->mLayer);
	this->shellWindow->setScope(this->mScope);
	this->mWayland->setKeyboardFocus(this->mKeyboardFocus);

	this->connected = true;
}

QQuickWindow* WaylandShellWindow::disownWindow() {
	QObject::disconnect(this->shellWindow, nullptr, this, nullptr);
	return this->ProxyWindowBase::disownWindow();
}

void WaylandShellWindow::setWidth(qint32 width) {
	this->mWidth = width;

	// only update the actual size if not blocked by anchors
	auto anchors = this->anchors();
	if (!anchors.mLeft || !anchors.mRight) this->ProxyShellWindow::setWidth(width);
}

void WaylandShellWindow::setHeight(qint32 height) {
	this->mHeight = height;

	// only update the actual size if not blocked by anchors
	auto anchors = this->anchors();
	if (!anchors.mTop || !anchors.mBottom) this->ProxyShellWindow::setHeight(height);
}

void WaylandShellWindow::setAnchors(Anchors anchors) {
	if (this->window == nullptr) {
		this->mAnchors = anchors;
		return;
	}

	auto lsAnchors = LayerShellQt::Window::Anchors();
	if (anchors.mLeft) lsAnchors |= LayerShellQt::Window::AnchorLeft;
	if (anchors.mRight) lsAnchors |= LayerShellQt::Window::AnchorRight;
	if (anchors.mTop) lsAnchors |= LayerShellQt::Window::AnchorTop;
	if (anchors.mBottom) lsAnchors |= LayerShellQt::Window::AnchorBottom;

	if (!anchors.mLeft || !anchors.mRight) this->ProxyWindowBase::setWidth(this->mWidth);
	if (!anchors.mTop || !anchors.mBottom) this->ProxyWindowBase::setHeight(this->mHeight);

	this->shellWindow->setAnchors(lsAnchors);
}

Anchors WaylandShellWindow::anchors() const {
	if (this->window == nullptr) return this->mAnchors;

	auto lsAnchors = this->shellWindow->anchors();

	Anchors anchors;
	anchors.mLeft = lsAnchors.testFlag(LayerShellQt::Window::AnchorLeft);
	anchors.mRight = lsAnchors.testFlag(LayerShellQt::Window::AnchorRight);
	anchors.mTop = lsAnchors.testFlag(LayerShellQt::Window::AnchorTop);
	anchors.mBottom = lsAnchors.testFlag(LayerShellQt::Window::AnchorBottom);

	return anchors;
}

void WaylandShellWindow::setExclusiveZone(qint32 zone) {
	if (zone < 0) zone = 0;
	if (this->connected && zone == this->mExclusionZone) return;
	this->mExclusionZone = zone;

	if (this->window != nullptr && this->exclusionMode() == ExclusionMode::Normal) {
		this->shellWindow->setExclusiveZone(zone);
		emit this->exclusionZoneChanged();
	}
}

qint32 WaylandShellWindow::exclusiveZone() const {
	if (this->window == nullptr) return this->mExclusionZone;
	else return this->shellWindow->exclusionZone();
}

ExclusionMode::Enum WaylandShellWindow::exclusionMode() const { return this->mExclusionMode; }

void WaylandShellWindow::setExclusionMode(ExclusionMode::Enum exclusionMode) {
	if (this->connected && exclusionMode == this->mExclusionMode) return;
	this->mExclusionMode = exclusionMode;

	if (this->window != nullptr) {
		if (exclusionMode == ExclusionMode::Normal) {
			this->shellWindow->setExclusiveZone(this->mExclusionZone);
			emit this->exclusionZoneChanged();
		} else if (exclusionMode == ExclusionMode::Ignore) {
			this->shellWindow->setExclusiveZone(-1);
			emit this->exclusionZoneChanged();
		} else {
			this->updateExclusionZone();
		}
	}
}

void WaylandShellWindow::setMargins(Margins margins) {
	if (this->window == nullptr) this->mMargins = margins;
	else {
		auto lsMargins = QMargins(margins.mLeft, margins.mTop, margins.mRight, margins.mBottom);
		this->shellWindow->setMargins(lsMargins);
	}
}

Margins WaylandShellWindow::margins() const {
	if (this->window == nullptr) return this->mMargins;
	auto lsMargins = this->shellWindow->margins();

	auto margins = Margins();
	margins.mLeft = lsMargins.left();
	margins.mRight = lsMargins.right();
	margins.mTop = lsMargins.top();
	margins.mBottom = lsMargins.bottom();

	return margins;
}

void WaylandShellWindowExtensions::setLayer(Layer::Enum layer) {
	if (this->window->window == nullptr) {
		this->window->mLayer = layer;
		return;
	}

	auto lsLayer = LayerShellQt::Window::LayerBackground;

	// clang-format off
	switch (layer) {
	case Layer::Background: lsLayer = LayerShellQt::Window::LayerBackground; break;
	case Layer::Bottom: lsLayer = LayerShellQt::Window::LayerBottom; break;
	case Layer::Top: lsLayer = LayerShellQt::Window::LayerTop; break;
	case Layer::Overlay: lsLayer = LayerShellQt::Window::LayerOverlay; break;
	}
	// clang-format on

	this->window->shellWindow->setLayer(lsLayer);
}

Layer::Enum WaylandShellWindowExtensions::layer() const {
	if (this->window->window == nullptr) return this->window->mLayer;

	auto layer = Layer::Top;
	auto lsLayer = this->window->shellWindow->layer();

	// clang-format off
	switch (lsLayer) {
	case LayerShellQt::Window::LayerBackground: layer = Layer::Background; break;
	case LayerShellQt::Window::LayerBottom: layer = Layer::Bottom; break;
	case LayerShellQt::Window::LayerTop: layer = Layer::Top; break;
	case LayerShellQt::Window::LayerOverlay: layer = Layer::Overlay; break;
	}
	// clang-format on

	return layer;
}

void WaylandShellWindowExtensions::setScope(const QString& scope) {
	if (this->window->window == nullptr) this->window->mScope = scope;
	else this->window->shellWindow->setScope(scope);
}

QString WaylandShellWindowExtensions::scope() const {
	if (this->window->window == nullptr) return this->window->mScope;
	else return this->window->shellWindow->scope();
}

void WaylandShellWindowExtensions::setKeyboardFocus(KeyboardFocus::Enum focus) {
	if (this->window->window == nullptr) {
		this->window->mKeyboardFocus = focus;
		return;
	}

	auto lsFocus = LayerShellQt::Window::KeyboardInteractivityNone;

	// clang-format off
	switch (focus) {
	case KeyboardFocus::None: lsFocus = LayerShellQt::Window::KeyboardInteractivityNone; break;
	case KeyboardFocus::Exclusive: lsFocus = LayerShellQt::Window::KeyboardInteractivityExclusive; break;
	case KeyboardFocus::OnDemand: lsFocus = LayerShellQt::Window::KeyboardInteractivityOnDemand; break;
	}
	// clang-format on

	this->window->shellWindow->setKeyboardInteractivity(lsFocus);
}

KeyboardFocus::Enum WaylandShellWindowExtensions::keyboardFocus() const {
	if (this->window->window == nullptr) return this->window->mKeyboardFocus;

	auto focus = KeyboardFocus::None;
	auto lsFocus = this->window->shellWindow->keyboardInteractivity();

	// clang-format off
	switch (lsFocus) {
	case LayerShellQt::Window::KeyboardInteractivityNone: focus = KeyboardFocus::None; break;
	case LayerShellQt::Window::KeyboardInteractivityExclusive: focus = KeyboardFocus::Exclusive; break;
	case LayerShellQt::Window::KeyboardInteractivityOnDemand: focus = KeyboardFocus::OnDemand; break;
	}
	// clang-format on

	return focus;
}

void WaylandShellWindowExtensions::setScreenConfiguration(ScreenConfiguration::Enum configuration) {
	if (this->window->window == nullptr) {
		this->window->mScreenConfiguration = configuration;
		return;
	}

	auto lsConfiguration = LayerShellQt::Window::ScreenFromQWindow;

	// clang-format off
	switch (configuration) {
	case ScreenConfiguration::Window: lsConfiguration = LayerShellQt::Window::ScreenFromQWindow; break;
	case ScreenConfiguration::Compositor: lsConfiguration = LayerShellQt::Window::ScreenFromCompositor; break;
	}
	// clang-format on

	this->window->shellWindow->setScreenConfiguration(lsConfiguration);
}

ScreenConfiguration::Enum WaylandShellWindowExtensions::screenConfiguration() const {
	if (this->window->window == nullptr) return this->window->mScreenConfiguration;

	auto configuration = ScreenConfiguration::Window;
	auto lsConfiguration = this->window->shellWindow->screenConfiguration();

	// clang-format off
	switch (lsConfiguration) {
	case LayerShellQt::Window::ScreenFromQWindow: configuration = ScreenConfiguration::Window; break;
	case LayerShellQt::Window::ScreenFromCompositor: configuration = ScreenConfiguration::Compositor; break;
	}
	// clang-format on

	return configuration;
}

void WaylandShellWindow::updateExclusionZone() {
	if (this->window != nullptr && this->exclusionMode() == ExclusionMode::Auto) {
		auto anchors = this->anchors();

		auto zone = -1;

		if (anchors.mTop && anchors.mBottom) {
			if (anchors.mLeft) zone = this->width() + this->margins().mLeft;
			else if (anchors.mRight) zone = this->width() + this->margins().mRight;
		} else if (anchors.mLeft && anchors.mRight) {
			if (anchors.mTop) zone = this->height() + this->margins().mTop;
			else if (anchors.mBottom) zone = this->height() + this->margins().mBottom;
		}

		if (zone != -1) {
			this->shellWindow->setExclusiveZone(zone);
			emit this->exclusionZoneChanged();
		}
	}
}
