#include "waylandshellwindow.hpp"

#include <qobject.h>
#include <qqmlcontext.h>
#include <qquickwindow.h>
#include <qtypes.h>
#include <qwindow.h>

#include "../core/proxywindow.hpp"
#include "../core/shellwindow.hpp"
#include "layershell.hpp"

WaylandShellWindow::WaylandShellWindow(QObject* parent)
    : ProxyShellWindow(parent)
    , mWayland(new WaylandShellWindowExtensions(this))
    , windowExtension(new LayershellWindowExtension(this)) {}

void WaylandShellWindow::setupWindow() {
	this->ProxyShellWindow::setupWindow();

	// clang-format off
	QObject::connect(this->windowExtension, &LayershellWindowExtension::anchorsChanged, this, &ProxyShellWindow::anchorsChanged);
	QObject::connect(this->windowExtension, &LayershellWindowExtension::marginsChanged, this, &ProxyShellWindow::marginsChanged);
	QObject::connect(this->windowExtension, &LayershellWindowExtension::exclusiveZoneChanged, this, &ProxyShellWindow::exclusionZoneChanged);

	QObject::connect(
	    this->windowExtension, &LayershellWindowExtension::layerChanged,
	    this->mWayland, &WaylandShellWindowExtensions::layerChanged
	);
	QObject::connect(
	    this->windowExtension, &LayershellWindowExtension::keyboardFocusChanged,
	    this->mWayland, &WaylandShellWindowExtensions::keyboardFocusChanged
	);

	QObject::connect(this->window, &QWindow::widthChanged, this, &WaylandShellWindow::updateExclusionZone);
	QObject::connect(this->window, &QWindow::heightChanged, this, &WaylandShellWindow::updateExclusionZone);
	QObject::connect(this, &ProxyShellWindow::anchorsChanged, this, &WaylandShellWindow::updateExclusionZone);
	QObject::connect(this, &ProxyShellWindow::marginsChanged, this, &WaylandShellWindow::updateExclusionZone);
	// clang-format on

	this->setMargins(this->mMargins);
	this->setExclusionMode(this->mExclusionMode); // also sets exclusion zone

	if (!this->windowExtension->attach(this->window)) {
		// todo: discard window
	}

	this->connected = true;
}

QQuickWindow* WaylandShellWindow::disownWindow() { return this->ProxyWindowBase::disownWindow(); }

void WaylandShellWindow::setWidth(qint32 width) {
	this->mWidth = width;

	// only update the actual size if not blocked by anchors
	if (!this->windowExtension->anchors().horizontalConstraint()) {
		this->ProxyShellWindow::setWidth(width);
	}
}

void WaylandShellWindow::onWidthChanged() {
	this->ProxyShellWindow::onWidthChanged();

	// change the remembered width only when not horizontally constrained.
	if (this->window != nullptr && !this->windowExtension->anchors().horizontalConstraint()) {
		this->mWidth = this->window->width();
	}
}

void WaylandShellWindow::setHeight(qint32 height) {
	this->mHeight = height;

	// only update the actual size if not blocked by anchors
	if (!this->windowExtension->anchors().verticalConstraint()) {
		this->ProxyShellWindow::setHeight(height);
	}
}

void WaylandShellWindow::onHeightChanged() {
	this->ProxyShellWindow::onHeightChanged();

	// change the remembered height only when not vertically constrained.
	if (this->window != nullptr && !this->windowExtension->anchors().verticalConstraint()) {
		this->mHeight = this->window->height();
	}
}

void WaylandShellWindow::setAnchors(Anchors anchors) {
	this->windowExtension->setAnchors(anchors);
	this->setHeight(this->mHeight);
	this->setWidth(this->mWidth);
}

Anchors WaylandShellWindow::anchors() const { return this->windowExtension->anchors(); }

void WaylandShellWindow::setExclusiveZone(qint32 zone) {
	this->windowExtension->setExclusiveZone(zone);
}

qint32 WaylandShellWindow::exclusiveZone() const { return this->windowExtension->exclusiveZone(); }

ExclusionMode::Enum WaylandShellWindow::exclusionMode() const { return this->mExclusionMode; }

void WaylandShellWindow::setExclusionMode(ExclusionMode::Enum exclusionMode) {
	if (this->connected && exclusionMode == this->mExclusionMode) return;
	this->mExclusionMode = exclusionMode;

	if (this->window != nullptr) {
		if (exclusionMode == ExclusionMode::Normal) {
			this->windowExtension->setExclusiveZone(this->mExclusionZone);
		} else if (exclusionMode == ExclusionMode::Ignore) {
			this->windowExtension->setExclusiveZone(-1);
		} else {
			this->updateExclusionZone();
		}
	}
}

void WaylandShellWindow::setMargins(Margins margins) { this->windowExtension->setMargins(margins); }

Margins WaylandShellWindow::margins() const { return this->windowExtension->margins(); }

void WaylandShellWindowExtensions::setLayer(Layer::Enum layer) {
	this->window->windowExtension->setLayer(layer);
}

Layer::Enum WaylandShellWindowExtensions::layer() const {
	return this->window->windowExtension->layer();
}

void WaylandShellWindowExtensions::setNamespace(const QString& ns) {
	if (this->window->windowExtension->isConfigured()) {
		auto* context = QQmlEngine::contextForObject(this);
		if (context != nullptr) {
			context->engine()->throwError(QString("Cannot change shell window namespace post-configure.")
			);
		}

		return;
	}

	this->window->windowExtension->setNamespace(ns);
}

QString WaylandShellWindowExtensions::ns() const { return this->window->windowExtension->ns(); }

void WaylandShellWindowExtensions::setKeyboardFocus(KeyboardFocus::Enum focus) {
	this->window->windowExtension->setKeyboardFocus(focus);
}

KeyboardFocus::Enum WaylandShellWindowExtensions::keyboardFocus() const {
	return this->window->windowExtension->keyboardFocus();
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
			this->windowExtension->setExclusiveZone(zone);
		}
	}
}
