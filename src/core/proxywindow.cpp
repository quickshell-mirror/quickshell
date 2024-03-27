#include "proxywindow.hpp"

#include <qnamespace.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qregion.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "generation.hpp"
#include "qmlglobal.hpp"
#include "qmlscreen.hpp"
#include "region.hpp"
#include "reload.hpp"

ProxyWindowBase::ProxyWindowBase(QObject* parent)
    : Reloadable(parent)
    , mContentItem(new QQuickItem()) {
	QQmlEngine::setObjectOwnership(this->mContentItem, QQmlEngine::CppOwnership);
	this->mContentItem->setParent(this);

	// clang-format off
	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::onWidthChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::onHeightChanged);

	QObject::connect(this, &ProxyWindowBase::maskChanged, this, &ProxyWindowBase::onMaskChanged);
	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::onMaskChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::onMaskChanged);

	QObject::connect(this, &ProxyWindowBase::xChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::yChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::backerVisibilityChanged, this, &ProxyWindowBase::windowTransformChanged);
	// clang-format on
}

ProxyWindowBase::~ProxyWindowBase() { this->deleteWindow(); }

void ProxyWindowBase::onReload(QObject* oldInstance) {
	this->window = this->retrieveWindow(oldInstance);
	auto wasVisible = this->window != nullptr && this->window->isVisible();
	if (this->window == nullptr) this->window = new QQuickWindow();

	Reloadable::reloadRecursive(this->mContentItem, oldInstance);

	this->connectWindow();
	this->completeWindow();

	this->reloadComplete = true;

	emit this->windowConnected();
	this->postCompleteWindow();

	if (wasVisible && this->isVisibleDirect()) emit this->backerVisibilityChanged();
}

void ProxyWindowBase::postCompleteWindow() { this->setVisible(this->mVisible); }

QQuickWindow* ProxyWindowBase::createQQuickWindow() { return new QQuickWindow(); }

void ProxyWindowBase::createWindow() {
	if (this->window != nullptr) return;
	this->window = this->createQQuickWindow();

	this->connectWindow();
	this->completeWindow();
	emit this->windowConnected();
}

void ProxyWindowBase::deleteWindow() {
	if (auto* window = this->disownWindow()) {
		if (auto* generation = EngineGeneration::findObjectGeneration(this)) {
			generation->deregisterIncubationController(window->incubationController());
		}

		window->deleteLater();
	}
}

QQuickWindow* ProxyWindowBase::disownWindow() {
	if (this->window == nullptr) return nullptr;

	QObject::disconnect(this->window, nullptr, this, nullptr);

	this->mContentItem->setParentItem(nullptr);

	auto* window = this->window;
	this->window = nullptr;
	return window;
}

QQuickWindow* ProxyWindowBase::retrieveWindow(QObject* oldInstance) {
	auto* old = qobject_cast<ProxyWindowBase*>(oldInstance);
	return old == nullptr ? nullptr : old->disownWindow();
}

void ProxyWindowBase::connectWindow() {
	if (auto* generation = EngineGeneration::findObjectGeneration(this)) {
		// All windows have effectively the same incubation controller so it dosen't matter
		// which window it belongs to. We do want to replace the delay one though.
		generation->registerIncubationController(this->window->incubationController());
	}

	// clang-format off
	QObject::connect(this->window, &QWindow::visibilityChanged, this, &ProxyWindowBase::visibleChanged);
	QObject::connect(this->window, &QWindow::xChanged, this, &ProxyWindowBase::xChanged);
	QObject::connect(this->window, &QWindow::yChanged, this, &ProxyWindowBase::yChanged);
	QObject::connect(this->window, &QWindow::widthChanged, this, &ProxyWindowBase::widthChanged);
	QObject::connect(this->window, &QWindow::heightChanged, this, &ProxyWindowBase::heightChanged);
	QObject::connect(this->window, &QWindow::screenChanged, this, &ProxyWindowBase::screenChanged);
	QObject::connect(this->window, &QQuickWindow::colorChanged, this, &ProxyWindowBase::colorChanged);
	// clang-format on
}

void ProxyWindowBase::completeWindow() {
	if (this->mScreen != nullptr && this->window->screen() != this->mScreen) {
		if (this->window->isVisible()) this->window->setVisible(false);
		this->window->setScreen(this->mScreen);
	}

	this->setWidth(this->mWidth);
	this->setHeight(this->mHeight);
	this->setColor(this->mColor);
	this->updateMask();

	// notify initial x and y positions
	emit this->xChanged();
	emit this->yChanged();

	this->mContentItem->setParentItem(this->window->contentItem());
	this->mContentItem->setWidth(this->width());
	this->mContentItem->setHeight(this->height());

	// without this the dangling screen pointer wont be updated to a real screen
	emit this->screenChanged();
}

bool ProxyWindowBase::deleteOnInvisible() const {
#ifdef NVIDIA_COMPAT
	// Nvidia drivers and Qt do not play nice when hiding and showing a window
	// so for nvidia compatibility we can never reuse windows if they have been
	// hidden.
	return true;
#else
	return false;
#endif
}

QQuickWindow* ProxyWindowBase::backingWindow() const { return this->window; }
QQuickItem* ProxyWindowBase::contentItem() const { return this->mContentItem; }

bool ProxyWindowBase::isVisible() const {
	if (this->window == nullptr) return this->mVisible;
	else return this->isVisibleDirect();
}

bool ProxyWindowBase::isVisibleDirect() const {
	if (this->window == nullptr) return false;
	else return this->window->isVisible();
}

void ProxyWindowBase::setVisible(bool visible) {
	this->mVisible = visible;
	if (this->reloadComplete) this->setVisibleDirect(visible);
}

void ProxyWindowBase::setVisibleDirect(bool visible) {
	if (this->deleteOnInvisible()) {
		if (visible == this->isVisibleDirect()) return;

		if (visible) {
			this->createWindow();
			this->window->setVisible(true);
			emit this->backerVisibilityChanged();
		} else {
			if (this->window != nullptr) {
				this->window->setVisible(false);
				emit this->backerVisibilityChanged();
				this->deleteWindow();
			}
		}
	} else if (this->window != nullptr) {
		this->window->setVisible(visible);
		emit this->backerVisibilityChanged();
	}
}

qint32 ProxyWindowBase::x() const {
	if (this->window == nullptr) return 0;
	else return this->window->x();
}

qint32 ProxyWindowBase::y() const {
	if (this->window == nullptr) return 0;
	else return this->window->y();
}

qint32 ProxyWindowBase::width() const {
	if (this->window == nullptr) return this->mWidth;
	else return this->window->width();
}

void ProxyWindowBase::setWidth(qint32 width) {
	this->mWidth = width;
	if (this->window == nullptr) {
		emit this->widthChanged();
	} else this->window->setWidth(width);
}

qint32 ProxyWindowBase::height() const {
	if (this->window == nullptr) return this->mHeight;
	else return this->window->height();
}

void ProxyWindowBase::setHeight(qint32 height) {
	this->mHeight = height;
	if (this->window == nullptr) {
		emit this->heightChanged();
	} else this->window->setHeight(height);
}

void ProxyWindowBase::setScreen(QuickshellScreenInfo* screen) {
	if (this->mScreen != nullptr) {
		QObject::disconnect(this->mScreen, nullptr, this, nullptr);
	}

	auto* qscreen = screen == nullptr ? nullptr : screen->screen;
	if (qscreen == this->mScreen) return;

	if (qscreen != nullptr) {
		QObject::connect(qscreen, &QObject::destroyed, this, &ProxyWindowBase::onScreenDestroyed);
	}

	if (this->window == nullptr) {
		this->mScreen = qscreen;
		emit this->screenChanged();
	} else {
		auto reshow = this->isVisibleDirect();
		if (reshow) this->setVisibleDirect(false);
		if (this->window != nullptr) this->window->setScreen(qscreen);
		if (reshow) this->setVisibleDirect(true);
	}
}

void ProxyWindowBase::onScreenDestroyed() { this->mScreen = nullptr; }

QuickshellScreenInfo* ProxyWindowBase::screen() const {
	QScreen* qscreen = nullptr;

	if (this->window == nullptr) {
		if (this->mScreen != nullptr) qscreen = this->mScreen;
	} else {
		qscreen = this->window->screen();
	}

	return QuickshellTracked::instance()->screenInfo(qscreen);
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
	if (mask == this->mMask) return;

	if (this->mMask != nullptr) {
		QObject::disconnect(this->mMask, nullptr, this, nullptr);
	}

	this->mMask = mask;

	if (mask != nullptr) {
		mask->setParent(this);
		QObject::connect(mask, &QObject::destroyed, this, &ProxyWindowBase::onMaskDestroyed);
		QObject::connect(mask, &PendingRegion::changed, this, &ProxyWindowBase::maskChanged);
	}

	emit this->maskChanged();
}

void ProxyWindowBase::onMaskChanged() {
	if (this->window != nullptr) this->updateMask();
}

void ProxyWindowBase::onMaskDestroyed() {
	this->mMask = nullptr;
	emit this->maskChanged();
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

	this->window->setFlag(Qt::WindowTransparentForInput, this->mMask != nullptr && mask.isEmpty());
	this->window->setMask(mask);
}

QQmlListProperty<QObject> ProxyWindowBase::data() {
	return this->mContentItem->property("data").value<QQmlListProperty<QObject>>();
}

void ProxyWindowBase::onWidthChanged() { this->mContentItem->setWidth(this->width()); }
void ProxyWindowBase::onHeightChanged() { this->mContentItem->setHeight(this->height()); }
