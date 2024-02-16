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
#include "qmlscreen.hpp"

void ProxyShellWindow::setupWindow() {
	QObject::connect(this->window, &QWindow::screenChanged, this, &ProxyShellWindow::screenChanged);
	this->shellWindow = LayerShellQt::Window::get(this->window);

	this->ProxyWindowBase::setupWindow();

	// clang-format off
	QObject::connect(this->shellWindow, &LayerShellQt::Window::anchorsChanged, this, &ProxyShellWindow::anchorsChanged);
	QObject::connect(this->shellWindow, &LayerShellQt::Window::marginsChanged, this, &ProxyShellWindow::marginsChanged);
	QObject::connect(this->shellWindow, &LayerShellQt::Window::layerChanged, this, &ProxyShellWindow::layerChanged);
	QObject::connect(this->shellWindow, &LayerShellQt::Window::keyboardInteractivityChanged, this, &ProxyShellWindow::keyboardFocusChanged);

	QObject::connect(this->window, &QWindow::widthChanged, this, &ProxyShellWindow::updateExclusionZone);
	QObject::connect(this->window, &QWindow::heightChanged, this, &ProxyShellWindow::updateExclusionZone);
	QObject::connect(this, &ProxyShellWindow::anchorsChanged, this, &ProxyShellWindow::updateExclusionZone);
	QObject::connect(this, &ProxyShellWindow::marginsChanged, this, &ProxyShellWindow::updateExclusionZone);
	// clang-format on

	this->window->setScreen(this->mScreen);
	this->setAnchors(this->mAnchors);
	this->setMargins(this->mMargins);
	this->setExclusionMode(this->mExclusionMode); // also sets exclusion zone
	this->setLayer(this->mLayer);
	this->shellWindow->setScope(this->mScope);
	this->setKeyboardFocus(this->mKeyboardFocus);

	this->connected = true;
}

QQuickWindow* ProxyShellWindow::disownWindow() {
	QObject::disconnect(this->shellWindow, nullptr, this, nullptr);
	return this->ProxyWindowBase::disownWindow();
}

void ProxyShellWindow::setWidth(qint32 width) {
	this->mWidth = width;

	// only update the actual size if not blocked by anchors
	auto anchors = this->anchors();
	if (!anchors.mLeft || !anchors.mRight) this->ProxyWindowBase::setWidth(width);
}

void ProxyShellWindow::setHeight(qint32 height) {
	this->mHeight = height;

	// only update the actual size if not blocked by anchors
	auto anchors = this->anchors();
	if (!anchors.mTop || !anchors.mBottom) this->ProxyWindowBase::setHeight(height);
}

void ProxyShellWindow::setScreen(QuickShellScreenInfo* screen) {
	if (this->mScreen != nullptr) {
		QObject::disconnect(this->mScreen, nullptr, this, nullptr);
	}

	auto* qscreen = screen == nullptr ? nullptr : screen->screen;
	if (qscreen != nullptr) {
		QObject::connect(qscreen, &QObject::destroyed, this, &ProxyShellWindow::onScreenDestroyed);
	}

	if (this->window == nullptr) this->mScreen = qscreen;
	else this->window->setScreen(qscreen);
}

void ProxyShellWindow::onScreenDestroyed() { this->mScreen = nullptr; }

QuickShellScreenInfo* ProxyShellWindow::screen() const {
	QScreen* qscreen = nullptr;

	if (this->window == nullptr) {
		if (this->mScreen != nullptr) qscreen = this->mScreen;
	} else {
		qscreen = this->window->screen();
	}

	return new QuickShellScreenInfo(
	    const_cast<ProxyShellWindow*>(this), // NOLINT
	    qscreen
	);
}

void ProxyShellWindow::setAnchors(Anchors anchors) {
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

Anchors ProxyShellWindow::anchors() const {
	if (this->window == nullptr) return this->mAnchors;

	auto lsAnchors = this->shellWindow->anchors();

	Anchors anchors;
	anchors.mLeft = lsAnchors.testFlag(LayerShellQt::Window::AnchorLeft);
	anchors.mRight = lsAnchors.testFlag(LayerShellQt::Window::AnchorRight);
	anchors.mTop = lsAnchors.testFlag(LayerShellQt::Window::AnchorTop);
	anchors.mBottom = lsAnchors.testFlag(LayerShellQt::Window::AnchorBottom);

	return anchors;
}

void ProxyShellWindow::setExclusiveZone(qint32 zone) {
	if (zone < 0) zone = 0;
	if (this->connected && zone == this->mExclusionZone) return;
	this->mExclusionZone = zone;

	if (this->window != nullptr && this->exclusionMode() == ExclusionMode::Normal) {
		this->shellWindow->setExclusiveZone(zone);
		emit this->exclusionZoneChanged();
	}
}

qint32 ProxyShellWindow::exclusiveZone() const {
	if (this->window == nullptr) return this->mExclusionZone;
	else return this->shellWindow->exclusionZone();
}

ExclusionMode::Enum ProxyShellWindow::exclusionMode() const { return this->mExclusionMode; }

void ProxyShellWindow::setExclusionMode(ExclusionMode::Enum exclusionMode) {
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

void ProxyShellWindow::setMargins(Margins margins) {
	if (this->window == nullptr) this->mMargins = margins;
	else {
		auto lsMargins = QMargins(margins.mLeft, margins.mTop, margins.mRight, margins.mBottom);
		this->shellWindow->setMargins(lsMargins);
	}
}

Margins ProxyShellWindow::margins() const {
	if (this->window == nullptr) return this->mMargins;
	auto lsMargins = this->shellWindow->margins();

	auto margins = Margins();
	margins.mLeft = lsMargins.left();
	margins.mRight = lsMargins.right();
	margins.mTop = lsMargins.top();
	margins.mBottom = lsMargins.bottom();

	return margins;
}

void ProxyShellWindow::setLayer(Layer::Enum layer) {
	if (this->window == nullptr) {
		this->mLayer = layer;
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

	this->shellWindow->setLayer(lsLayer);
}

Layer::Enum ProxyShellWindow::layer() const {
	if (this->window == nullptr) return this->mLayer;

	auto layer = Layer::Top;
	auto lsLayer = this->shellWindow->layer();

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

void ProxyShellWindow::setScope(const QString& scope) {
	if (this->window == nullptr) this->mScope = scope;
	else this->shellWindow->setScope(scope);
}

QString ProxyShellWindow::scope() const {
	if (this->window == nullptr) return this->mScope;
	else return this->shellWindow->scope();
}

void ProxyShellWindow::setKeyboardFocus(KeyboardFocus::Enum focus) {
	if (this->window == nullptr) {
		this->mKeyboardFocus = focus;
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

	this->shellWindow->setKeyboardInteractivity(lsFocus);
}

KeyboardFocus::Enum ProxyShellWindow::keyboardFocus() const {
	if (this->window == nullptr) return this->mKeyboardFocus;

	auto focus = KeyboardFocus::None;
	auto lsFocus = this->shellWindow->keyboardInteractivity();

	// clang-format off
	switch (lsFocus) {
	case LayerShellQt::Window::KeyboardInteractivityNone: focus = KeyboardFocus::None; break;
	case LayerShellQt::Window::KeyboardInteractivityExclusive: focus = KeyboardFocus::Exclusive; break;
	case LayerShellQt::Window::KeyboardInteractivityOnDemand: focus = KeyboardFocus::OnDemand; break;
	}
	// clang-format on

	return focus;
}

void ProxyShellWindow::setScreenConfiguration(ScreenConfiguration::Enum configuration) {
	if (this->window == nullptr) {
		this->mScreenConfiguration = configuration;
		return;
	}

	auto lsConfiguration = LayerShellQt::Window::ScreenFromQWindow;

	// clang-format off
	switch (configuration) {
	case ScreenConfiguration::Window: lsConfiguration = LayerShellQt::Window::ScreenFromQWindow; break;
	case ScreenConfiguration::Compositor: lsConfiguration = LayerShellQt::Window::ScreenFromCompositor; break;
	}
	// clang-format on

	this->shellWindow->setScreenConfiguration(lsConfiguration);
}

ScreenConfiguration::Enum ProxyShellWindow::screenConfiguration() const {
	if (this->window == nullptr) return this->mScreenConfiguration;

	auto configuration = ScreenConfiguration::Window;
	auto lsConfiguration = this->shellWindow->screenConfiguration();

	// clang-format off
	switch (lsConfiguration) {
	case LayerShellQt::Window::ScreenFromQWindow: configuration = ScreenConfiguration::Window; break;
	case LayerShellQt::Window::ScreenFromCompositor: configuration = ScreenConfiguration::Compositor; break;
	}
	// clang-format on

	return configuration;
}

void ProxyShellWindow::updateExclusionZone() {
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
