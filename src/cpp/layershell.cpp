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

void ProxyShellWindow::earlyInit(QObject* old) {
	this->ProxyWindowBase::earlyInit(old);

	QObject::connect(this->window, &QWindow::screenChanged, this, &ProxyShellWindow::screenChanged);

	this->shellWindow = LayerShellQt::Window::get(this->window);

	// dont want to steal focus unless actually configured to
	this->shellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);

	// this dosent reset if its unset
	this->shellWindow->setExclusiveZone(0);

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
}

void ProxyShellWindow::componentComplete() {
	this->complete = true;

	// The default anchor settings are a hazard because they cover the entire screen.
	// We opt for 0 anchors by default to avoid blocking user input.
	this->setAnchors(this->stagingAnchors);
	this->updateExclusionZone();

	// Make sure we signal changes from anchors, but only if this is a reload.
	// If we do it on first load then it sends an extra change at 0px.
	if (this->stagingAnchors.mLeft && this->stagingAnchors.mRight && this->width() != 0)
		this->widthChanged(this->width());
	if (this->stagingAnchors.mTop && this->stagingAnchors.mBottom && this->height() != 0)
		this->heightChanged(this->height());

	this->window->setVisible(this->stagingVisible);

	this->ProxyWindowBase::componentComplete();
}

QQuickWindow* ProxyShellWindow::disownWindow() {
	QObject::disconnect(this->shellWindow, nullptr, this, nullptr);
	return this->ProxyWindowBase::disownWindow();
}

void ProxyShellWindow::setVisible(bool visible) {
	if (!this->complete) this->stagingVisible = visible;
	else this->ProxyWindowBase::setVisible(visible);
}

bool ProxyShellWindow::isVisible() {
	return this->complete ? this->ProxyWindowBase::isVisible() : this->stagingVisible;
}

void ProxyShellWindow::setWidth(qint32 width) {
	this->requestedWidth = width;

	// only update the actual size if not blocked by anchors
	auto anchors = this->anchors();
	if (this->complete && (!anchors.mLeft || !anchors.mRight)) this->ProxyWindowBase::setWidth(width);
}

qint32 ProxyShellWindow::width() {
	return this->complete ? this->ProxyWindowBase::width() : this->requestedWidth;
}

void ProxyShellWindow::setHeight(qint32 height) {
	this->requestedHeight = height;

	// only update the actual size if not blocked by anchors
	auto anchors = this->anchors();
	if (this->complete && (!anchors.mTop || !anchors.mBottom))
		this->ProxyWindowBase::setHeight(height);
}

qint32 ProxyShellWindow::height() {
	return this->complete ? this->ProxyWindowBase::height() : this->requestedHeight;
}

void ProxyShellWindow::setScreen(QuickShellScreenInfo* screen) {
	this->window->setScreen(screen->screen);
}

QuickShellScreenInfo* ProxyShellWindow::screen() const {
	return new QuickShellScreenInfo(
	    const_cast<ProxyShellWindow*>(this), // NOLINT
	    this->window->screen()
	);
}

void ProxyShellWindow::setAnchors(Anchors anchors) {
	if (!this->complete) {
		this->stagingAnchors = anchors;
		return;
	}

	auto lsAnchors = LayerShellQt::Window::Anchors();
	if (anchors.mLeft) lsAnchors |= LayerShellQt::Window::AnchorLeft;
	if (anchors.mRight) lsAnchors |= LayerShellQt::Window::AnchorRight;
	if (anchors.mTop) lsAnchors |= LayerShellQt::Window::AnchorTop;
	if (anchors.mBottom) lsAnchors |= LayerShellQt::Window::AnchorBottom;

	if (!anchors.mLeft || !anchors.mRight) this->ProxyWindowBase::setWidth(this->requestedWidth);
	if (!anchors.mTop || !anchors.mBottom) this->ProxyWindowBase::setHeight(this->requestedHeight);

	this->shellWindow->setAnchors(lsAnchors);
}

Anchors ProxyShellWindow::anchors() const {
	if (!this->complete) return this->stagingAnchors;

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
	if (zone == this->requestedExclusionZone) return;
	this->requestedExclusionZone = zone;

	if (this->exclusionMode() == ExclusionMode::Normal) {
		this->shellWindow->setExclusiveZone(zone);
		emit this->exclusionZoneChanged();
	}
}

qint32 ProxyShellWindow::exclusiveZone() const { return this->shellWindow->exclusionZone(); }

ExclusionMode::Enum ProxyShellWindow::exclusionMode() const { return this->mExclusionMode; }

void ProxyShellWindow::setExclusionMode(ExclusionMode::Enum exclusionMode) {
	if (exclusionMode == this->mExclusionMode) return;
	this->mExclusionMode = exclusionMode;

	if (exclusionMode == ExclusionMode::Normal) {
		this->shellWindow->setExclusiveZone(this->requestedExclusionZone);
		emit this->exclusionZoneChanged();
	} else if (exclusionMode == ExclusionMode::Ignore) {
		this->shellWindow->setExclusiveZone(-1);
		emit this->exclusionZoneChanged();
	} else {
		this->updateExclusionZone();
	}
}

void ProxyShellWindow::setMargins(Margins margins) {
	auto lsMargins = QMargins(margins.mLeft, margins.mTop, margins.mRight, margins.mBottom);
	this->shellWindow->setMargins(lsMargins);
}

Margins ProxyShellWindow::margins() const {
	auto lsMargins = this->shellWindow->margins();

	auto margins = Margins();
	margins.mLeft = lsMargins.left();
	margins.mRight = lsMargins.right();
	margins.mTop = lsMargins.top();
	margins.mBottom = lsMargins.bottom();

	return margins;
}

void ProxyShellWindow::setLayer(Layer::Enum layer) {
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

void ProxyShellWindow::setScope(const QString& scope) { this->shellWindow->setScope(scope); }

QString ProxyShellWindow::scope() const { return this->shellWindow->scope(); }

void ProxyShellWindow::setKeyboardFocus(KeyboardFocus::Enum focus) {
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

void ProxyShellWindow::setCloseOnDismissed(bool close) {
	this->shellWindow->setCloseOnDismissed(close);
}

bool ProxyShellWindow::closeOnDismissed() const { return this->shellWindow->closeOnDismissed(); }

void ProxyShellWindow::updateExclusionZone() {
	if (this->exclusionMode() == ExclusionMode::Auto) {
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
