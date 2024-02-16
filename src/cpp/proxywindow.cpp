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

	auto backer = this->dataBacker();
	for (auto* child: this->pendingChildren) {
		backer.append(&backer, child);
	}

	this->pendingChildren.clear();

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

	auto data = this->data();
	ProxyWindowBase::dataClear(&data);
	data.clear(&data);

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

// see:
// https://code.qt.io/cgit/qt/qtdeclarative.git/tree/src/quick/items/qquickwindow.cpp
// https://code.qt.io/cgit/qt/qtdeclarative.git/tree/src/quick/items/qquickitem.cpp
//
// relevant functions are private so we call them via the property

QQmlListProperty<QObject> ProxyWindowBase::data() {
	return QQmlListProperty<QObject>(
	    this,
	    nullptr,
	    ProxyWindowBase::dataAppend,
	    ProxyWindowBase::dataCount,
	    ProxyWindowBase::dataAt,
	    ProxyWindowBase::dataClear,
	    ProxyWindowBase::dataReplace,
	    ProxyWindowBase::dataRemoveLast
	);
}

QQmlListProperty<QObject> ProxyWindowBase::dataBacker() {
	return this->window->property("data").value<QQmlListProperty<QObject>>();
}

void ProxyWindowBase::dataAppend(QQmlListProperty<QObject>* prop, QObject* obj) {
	auto* self = static_cast<ProxyWindowBase*>(prop->object); // NOLINT

	if (self->window == nullptr) {
		if (obj != nullptr) {
			obj->setParent(self);
			self->pendingChildren.append(obj);
		}
	} else {
		auto backer = self->dataBacker();
		backer.append(&backer, obj);
	}
}

qsizetype ProxyWindowBase::dataCount(QQmlListProperty<QObject>* prop) {
	auto* self = static_cast<ProxyWindowBase*>(prop->object); // NOLINT

	if (self->window == nullptr) {
		return self->pendingChildren.count();
	} else {
		auto backer = self->dataBacker();
		return backer.count(&backer);
	}
}

QObject* ProxyWindowBase::dataAt(QQmlListProperty<QObject>* prop, qsizetype i) {
	auto* self = static_cast<ProxyWindowBase*>(prop->object); // NOLINT

	if (self->window == nullptr) {
		return self->pendingChildren.at(i);
	} else {
		auto backer = self->dataBacker();
		return backer.at(&backer, i);
	}
}

void ProxyWindowBase::dataClear(QQmlListProperty<QObject>* prop) {
	auto* self = static_cast<ProxyWindowBase*>(prop->object); // NOLINT

	if (self->window == nullptr) {
		self->pendingChildren.clear();
	} else {
		auto backer = self->dataBacker();
		backer.clear(&backer);
	}
}

void ProxyWindowBase::dataReplace(QQmlListProperty<QObject>* prop, qsizetype i, QObject* obj) {
	auto* self = static_cast<ProxyWindowBase*>(prop->object); // NOLINT

	if (self->window == nullptr) {
		if (obj != nullptr) {
			obj->setParent(self);
			self->pendingChildren.replace(i, obj);
		}
	} else {
		auto backer = self->dataBacker();
		backer.replace(&backer, i, obj);
	}
}

void ProxyWindowBase::dataRemoveLast(QQmlListProperty<QObject>* prop) {
	auto* self = static_cast<ProxyWindowBase*>(prop->object); // NOLINT

	if (self->window == nullptr) {
		self->pendingChildren.removeLast();
	} else {
		auto backer = self->dataBacker();
		backer.removeLast(&backer);
	}
}

void ProxyFloatingWindow::setWidth(qint32 width) {
	if (this->window == nullptr || !this->window->isVisible()) this->ProxyWindowBase::setWidth(width);
}

void ProxyFloatingWindow::setHeight(qint32 height) {
	if (this->window == nullptr || !this->window->isVisible())
		this->ProxyWindowBase::setHeight(height);
}
