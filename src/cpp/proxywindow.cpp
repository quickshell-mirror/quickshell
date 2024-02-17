#include "proxywindow.hpp"

#include <qobject.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qregion.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "region.hpp"
#include "reload.hpp"

ProxyWindowBase::ProxyWindowBase(QObject* parent): Reloadable(parent) {
	this->contentItem = new QQuickItem(); // NOLINT
	this->contentItem->setParent(this);

	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::onWidthChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::onHeightChanged);
}

ProxyWindowBase::~ProxyWindowBase() {
	if (this->window != nullptr) {
		this->window->deleteLater();
	}
}

void ProxyWindowBase::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<ProxyWindowBase*>(oldInstance);

	if (old == nullptr || old->window == nullptr) {
		this->window = new QQuickWindow();
	} else {
		this->window = old->disownWindow();
	}

	this->setupWindow();

	Reloadable::reloadRecursive(this->contentItem, oldInstance);

	this->contentItem->setParentItem(this->window->contentItem());

	this->contentItem->setWidth(this->width());
	this->contentItem->setHeight(this->height());

	emit this->windowConnected();
	this->window->setVisible(this->mVisible);
}

void ProxyWindowBase::setupWindow() {
	// clang-format off
	QObject::connect(this->window, &QWindow::visibilityChanged, this, &ProxyWindowBase::visibleChanged);
	QObject::connect(this->window, &QWindow::widthChanged, this, &ProxyWindowBase::widthChanged);
	QObject::connect(this->window, &QWindow::heightChanged, this, &ProxyWindowBase::heightChanged);
	QObject::connect(this->window, &QQuickWindow::colorChanged, this, &ProxyWindowBase::colorChanged);

	QObject::connect(this, &ProxyWindowBase::maskChanged, this, &ProxyWindowBase::onMaskChanged);
	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::onMaskChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::onMaskChanged);
	// clang-format on

	this->setWidth(this->mWidth);
	this->setHeight(this->mHeight);
	this->setColor(this->mColor);
	this->updateMask();
}

QQuickWindow* ProxyWindowBase::disownWindow() {
	QObject::disconnect(this->window, nullptr, this, nullptr);

	this->contentItem->setParentItem(nullptr);

	auto* window = this->window;
	this->window = nullptr;
	return window;
}

QQuickWindow* ProxyWindowBase::backingWindow() const { return this->window; }

bool ProxyWindowBase::isVisible() const {
	if (this->window == nullptr) return this->mVisible;
	else return this->window->isVisible();
}

void ProxyWindowBase::setVisible(bool visible) {
	if (this->window == nullptr) {
		this->mVisible = visible;
		emit this->visibleChanged();
	} else this->window->setVisible(visible);
}

qint32 ProxyWindowBase::width() const {
	if (this->window == nullptr) return this->mWidth;
	else return this->window->width();
}

void ProxyWindowBase::setWidth(qint32 width) {
	if (this->window == nullptr) {
		this->mWidth = width;
		emit this->widthChanged();
	} else this->window->setWidth(width);
}

qint32 ProxyWindowBase::height() const {
	if (this->window == nullptr) return this->mHeight;
	else return this->window->height();
}

void ProxyWindowBase::setHeight(qint32 height) {
	if (this->window == nullptr) {
		this->mHeight = height;
		emit this->heightChanged();
	} else this->window->setHeight(height);
}

QColor ProxyWindowBase::color() const {
	if (this->window == nullptr) return this->mColor;
	else return this->window->color();
}

void ProxyWindowBase::setColor(QColor color) {
	if (this->window == nullptr) {
		this->mColor = color;
		emit this->colorChanged();
	} else this->window->setColor(color);
}

PendingRegion* ProxyWindowBase::mask() const { return this->mMask; }

void ProxyWindowBase::setMask(PendingRegion* mask) {
	if (this->mMask != nullptr) {
		this->mMask->deleteLater();
	}

	if (mask != nullptr) {
		mask->setParent(this);
		this->mMask = mask;
		QObject::connect(mask, &PendingRegion::changed, this, &ProxyWindowBase::maskChanged);
		emit this->maskChanged();
	}
}

void ProxyWindowBase::onMaskChanged() {
	if (this->window != nullptr) this->updateMask();
}

void ProxyWindowBase::updateMask() {
	QRegion mask;
	if (this->mMask != nullptr) {
		// if left as the default, dont combine it with the whole window area, leave it as is.
		if (this->mMask->mIntersection == Intersection::Combine) {
			mask = this->mMask->build();
		} else {
			auto windowRegion = QRegion(QRect(0, 0, this->width(), this->height()));
			mask = this->mMask->applyTo(windowRegion);
		}
	}

	this->window->setMask(mask);
}

QQmlListProperty<QObject> ProxyWindowBase::data() {
	return this->contentItem->property("data").value<QQmlListProperty<QObject>>();
}

void ProxyWindowBase::onWidthChanged() { this->contentItem->setWidth(this->width()); }

void ProxyWindowBase::onHeightChanged() { this->contentItem->setHeight(this->height()); }

void ProxyFloatingWindow::setWidth(qint32 width) {
	if (this->window == nullptr || !this->window->isVisible()) this->ProxyWindowBase::setWidth(width);
}

void ProxyFloatingWindow::setHeight(qint32 height) {
	if (this->window == nullptr || !this->window->isVisible())
		this->ProxyWindowBase::setHeight(height);
}
