#include "proxywindow.hpp"

#include <private/qquickwindow_p.h>
#include <qcontainerfwd.h>
#include <qcoreevent.h>
#include <qevent.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qqmllist.h>
#include <qquickgraphicsconfiguration.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qregion.h>
#include <qsurfaceformat.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindow.h>

#include "../core/generation.hpp"
#include "../core/qmlglobal.hpp"
#include "../core/qmlscreen.hpp"
#include "../core/region.hpp"
#include "../core/reload.hpp"
#include "../debug/lint.hpp"
#include "windowinterface.hpp"

ProxyWindowBase::ProxyWindowBase(QObject* parent)
    : Reloadable(parent)
    , mContentItem(new ProxyWindowContentItem()) {
	QQmlEngine::setObjectOwnership(this->mContentItem, QQmlEngine::CppOwnership);
	this->mContentItem->setParent(this);

	// clang-format off
	QObject::connect(this->mContentItem, &ProxyWindowContentItem::polished, this, &ProxyWindowBase::onPolished);

	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::onWidthChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::onHeightChanged);

	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::onMaskChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::onMaskChanged);

	QObject::connect(this, &ProxyWindowBase::xChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::yChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::widthChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::heightChanged, this, &ProxyWindowBase::windowTransformChanged);
	QObject::connect(this, &ProxyWindowBase::backerVisibilityChanged, this, &ProxyWindowBase::windowTransformChanged);
	// clang-format on
}

ProxyWindowBase::~ProxyWindowBase() { this->deleteWindow(true); }

void ProxyWindowBase::onReload(QObject* oldInstance) {
	if (this->mVisible) this->window = this->retrieveWindow(oldInstance);
	auto wasVisible = this->window != nullptr && this->window->isVisible();

	if (this->mVisible) this->ensureQWindow();

	// The qml engine will leave the WindowInterface as owner of everything
	// nested in an item, so we have to make sure the interface's children
	// are also reloaded.
	// Reparenting from the interface does not work reliably, so instead
	// we check if the parent is one, as it proxies reloads to here.
	if (auto* w = qobject_cast<WindowInterface*>(this->parent())) {
		for (auto* child: w->children()) {
			if (child == this) continue;
			auto* oldInterfaceParent = oldInstance == nullptr ? nullptr : oldInstance->parent();
			Reloadable::reloadRecursive(child, oldInterfaceParent);
		}
	}

	Reloadable::reloadChildrenRecursive(this, oldInstance);

	if (this->mVisible) {
		this->connectWindow();
		this->completeWindow();
	}

	this->reloadComplete = true;

	if (this->mVisible) {
		emit this->windowConnected();
		this->postCompleteWindow();

		if (wasVisible && this->isVisibleDirect()) {
			this->bBackerVisibility = true;
			this->onExposed();
		}
	}
}

void ProxyWindowBase::postCompleteWindow() { this->setVisible(this->mVisible); }

ProxiedWindow* ProxyWindowBase::createQQuickWindow() { return new ProxiedWindow(this); }

void ProxyWindowBase::ensureQWindow() {
	auto format = QSurfaceFormat::defaultFormat();

	{
		// match QtQuick's default format, including env var controls
		static const auto useDepth = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
		static const auto useStencil = qEnvironmentVariableIsEmpty("QSG_NO_STENCIL_BUFFER");
		static const auto enableDebug = qEnvironmentVariableIsSet("QSG_OPENGL_DEBUG");
		static const auto disableVSync = qEnvironmentVariableIsSet("QSG_NO_VSYNC");

		if (useDepth && format.depthBufferSize() == -1) format.setDepthBufferSize(24);
		else if (!useDepth) format.setDepthBufferSize(0);

		if (useStencil && format.stencilBufferSize() == -1) format.setStencilBufferSize(8);
		else if (!useStencil) format.setStencilBufferSize(0);

		auto opaque = this->qsSurfaceFormat.opaqueModified ? this->qsSurfaceFormat.opaque
		                                                   : this->mColor.alpha() >= 255;

		format.setOption(QSurfaceFormat::ResetNotification);

		if (opaque) format.setAlphaBufferSize(0);
		else format.setAlphaBufferSize(8);

		if (enableDebug) format.setOption(QSurfaceFormat::DebugContext);
		if (disableVSync) format.setSwapInterval(0);

		format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
		format.setRedBufferSize(8);
		format.setGreenBufferSize(8);
		format.setBlueBufferSize(8);
	}

	this->mSurfaceFormat = format;

	auto useOldWindow = this->window != nullptr;

	if (useOldWindow) {
		if (this->window->requestedFormat() != format) {
			useOldWindow = false;
		}
	}

	if (useOldWindow) return;
	delete this->window;
	this->window = nullptr; // createQQuickWindow may indirectly reference this->window
	this->window = this->createQQuickWindow();
	this->window->setFormat(format);

	// needed for vulkan dmabuf import, qt ignores these if not applicable
	auto graphicsConfig = this->window->graphicsConfiguration();
	graphicsConfig.setDeviceExtensions({
	    "VK_KHR_external_memory_fd",
	    "VK_EXT_external_memory_dma_buf",
	    "VK_EXT_image_drm_format_modifier",
	});
	this->window->setGraphicsConfiguration(graphicsConfig);
}

void ProxyWindowBase::createWindow() {
	this->ensureQWindow();
	this->connectWindow();
	this->completeWindow();
	emit this->windowConnected();
}

void ProxyWindowBase::deleteWindow(bool keepItemOwnership) {
	if (this->window != nullptr) emit this->windowDestroyed();
	if (auto* window = this->disownWindow(keepItemOwnership)) window->deleteLater();
}

ProxiedWindow* ProxyWindowBase::disownWindow(bool keepItemOwnership) {
	if (this->window == nullptr) return nullptr;

	QObject::disconnect(this->window, nullptr, this, nullptr);

	if (!keepItemOwnership) {
		this->mContentItem->setParentItem(nullptr);
	}

	auto* window = this->window;
	this->window = nullptr;
	return window;
}

ProxiedWindow* ProxyWindowBase::retrieveWindow(QObject* oldInstance) {
	auto* old = qobject_cast<ProxyWindowBase*>(oldInstance);
	return old == nullptr ? nullptr : old->disownWindow();
}

void ProxyWindowBase::connectWindow() {
	if (auto* generation = EngineGeneration::findObjectGeneration(this)) {
		// All windows have effectively the same incubation controller so it dosen't matter
		// which window it belongs to. We do want to replace the delay one though.
		generation->trackWindowIncubationController(this->window);
	}

	this->window->setProxy(this);

	// clang-format off
	QObject::connect(this->window, &QWindow::visibilityChanged, this, &ProxyWindowBase::onVisibleChanged);
	QObject::connect(this->window, &QWindow::xChanged, this, &ProxyWindowBase::xChanged);
	QObject::connect(this->window, &QWindow::yChanged, this, &ProxyWindowBase::yChanged);
	QObject::connect(this->window, &QWindow::widthChanged, this, &ProxyWindowBase::widthChanged);
	QObject::connect(this->window, &QWindow::heightChanged, this, &ProxyWindowBase::heightChanged);
	QObject::connect(this->window, &QWindow::screenChanged, this, &ProxyWindowBase::screenChanged);
	QObject::connect(this->window, &QQuickWindow::colorChanged, this, &ProxyWindowBase::colorChanged);
	QObject::connect(this->window, &QQuickWindow::sceneGraphError, this, &ProxyWindowBase::onSceneGraphError);
	QObject::connect(this->window, &ProxiedWindow::exposed, this, &ProxyWindowBase::onExposed);
	QObject::connect(this->window, &ProxiedWindow::devicePixelRatioChanged, this, &ProxyWindowBase::devicePixelRatioChanged);
	// clang-format on
}

void ProxyWindowBase::completeWindow() {
	if (this->mScreen != nullptr && this->window->screen() != this->mScreen) {
		if (this->window->isVisible()) this->window->setVisible(false);
		this->window->setScreen(this->mScreen);
	}

	this->trySetWidth(this->implicitWidth());
	this->trySetHeight(this->implicitHeight());
	this->setColor(this->mColor);
	this->updateMask();

	// notify initial / post-connection geometry
	emit this->xChanged();
	emit this->yChanged();
	emit this->widthChanged();
	emit this->heightChanged();
	emit this->devicePixelRatioChanged();

	this->mContentItem->setParentItem(this->window->contentItem());
	this->mContentItem->setWidth(this->width());
	this->mContentItem->setHeight(this->height());

	// without this the dangling screen pointer wont be updated to a real screen
	emit this->screenChanged();
}

void ProxyWindowBase::onSceneGraphError(
    QQuickWindow::SceneGraphError error,
    const QString& message
) {
	if (error == QQuickWindow::ContextNotAvailable) {
		qCritical().nospace() << "Failed to create graphics context for " << this << ": " << message;
	} else {
		qCritical().nospace() << "Scene graph error " << error << " occurred for " << this << ": "
		                      << message;
	}

	emit this->resourcesLost();
	this->mVisible = false;
	this->setVisibleDirect(false);
}

void ProxyWindowBase::onVisibleChanged() {
	if (this->mVisible && !this->window->isVisible()) {
		this->mVisible = false;
		this->setVisibleDirect(false);
		emit this->closed();
	}

	emit this->visibleChanged();
}

bool ProxyWindowBase::deleteOnInvisible() const { return false; }

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
		if (visible) {
			if (visible == this->isVisibleDirect()) return;
			this->createWindow();
			this->polishItems();
			this->window->setVisible(true);
			this->bBackerVisibility = true;
		} else {
			if (this->window != nullptr) this->window->setVisible(false);
			this->bBackerVisibility = false;
			this->deleteWindow();
		}
	} else {
		if (visible && this->window == nullptr) {
			this->createWindow();
		}

		if (this->window != nullptr) {
			if (visible) this->polishItems();
			this->window->setVisible(visible);
			this->bBackerVisibility = visible;
		}
	}
}

void ProxyWindowBase::schedulePolish() {
	if (this->isVisibleDirect()) {
		this->mContentItem->polish();
	}
}

void ProxyWindowBase::polishItems() {
	// Due to QTBUG-126704, layouts in invisible windows don't update their dimensions.
	// Usually this isn't an issue, but it is when the size of a window is based on the size
	// of its content, and that content is in a layout.
	//
	// This hack manually polishes the item tree right before showing the window so it will
	// always be created with the correct size.
	QQuickWindowPrivate::get(this->window)->polishItems();
}

void ProxyWindowBase::onExposed() {
	this->onPolished();

	if (!this->ranLints) {
		qs::debug::lintItemTree(this->mContentItem);
		this->ranLints = true;
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

void ProxyWindowBase::setImplicitWidth(qint32 implicitWidth) {
	if (implicitWidth == this->bImplicitWidth) return;
	this->bImplicitWidth = implicitWidth;

	if (this->window) this->trySetWidth(implicitWidth);
	else emit this->widthChanged();
}

void ProxyWindowBase::trySetWidth(qint32 implicitWidth) { this->window->setWidth(implicitWidth); }

void ProxyWindowBase::setImplicitHeight(qint32 implicitHeight) {
	if (implicitHeight == this->bImplicitHeight) return;
	this->bImplicitHeight = implicitHeight;

	if (this->window) this->trySetHeight(implicitHeight);
	else emit this->heightChanged();
}

void ProxyWindowBase::trySetHeight(qint32 implicitHeight) {
	this->window->setHeight(implicitHeight);
}

qint32 ProxyWindowBase::width() const {
	if (this->window == nullptr) return this->implicitWidth();
	else return this->window->width();
}

void ProxyWindowBase::setWidth(qint32 width) {
	this->setImplicitWidth(width);
	qmlWarning(this) << "Setting `width` is deprecated. Set `implicitWidth` instead.";
}

qint32 ProxyWindowBase::height() const {
	if (this->window == nullptr) return this->implicitHeight();
	else return this->window->height();
}

void ProxyWindowBase::setHeight(qint32 height) {
	this->setImplicitHeight(height);
	qmlWarning(this) << "Setting `height` is deprecated. Set `implicitHeight` instead.";
}

void ProxyWindowBase::setScreen(QuickshellScreenInfo* screen) {
	auto* qscreen = screen == nullptr ? nullptr : screen->screen;
	auto newMScreen = this->mScreen != qscreen;

	if (this->mScreen && newMScreen) {
		QObject::disconnect(this->mScreen, nullptr, this, nullptr);
	}

	auto* oldScreen = this->qscreen();
	this->mScreen = qscreen;

	if (oldScreen != qscreen) {
		if (this->window == nullptr) {
			emit this->screenChanged();
		} else if (qscreen) {
			auto reshow = this->isVisibleDirect();
			if (reshow) this->setVisibleDirect(false);
			if (this->window != nullptr) this->window->setScreen(qscreen);
			if (reshow) this->setVisibleDirect(true);
		}
	}

	if (qscreen && newMScreen) {
		QObject::connect(this->mScreen, &QObject::destroyed, this, &ProxyWindowBase::onScreenDestroyed);
	}
}

void ProxyWindowBase::onScreenDestroyed() { this->mScreen = nullptr; }

QScreen* ProxyWindowBase::qscreen() const {
	if (this->window) return this->window->screen();
	if (this->mScreen) return this->mScreen;
	return QGuiApplication::primaryScreen();
}

QuickshellScreenInfo* ProxyWindowBase::screen() const {
	return QuickshellTracked::instance()->screenInfo(this->qscreen());
}

QColor ProxyWindowBase::color() const { return this->mColor; }

void ProxyWindowBase::setColor(QColor color) {
	this->mColor = color;

	if (this->window == nullptr) {
		if (color != this->mColor) emit this->colorChanged();
	} else {
		auto premultiplied = QColor::fromRgbF(
		    color.redF() * color.alphaF(),
		    color.greenF() * color.alphaF(),
		    color.blueF() * color.alphaF(),
		    color.alphaF()
		);

		this->window->setColor(premultiplied);
		// setColor also modifies the alpha buffer size of the surface format
		this->window->setFormat(this->mSurfaceFormat);
	}
}

PendingRegion* ProxyWindowBase::mask() const { return this->mMask; }

void ProxyWindowBase::setMask(PendingRegion* mask) {
	if (mask == this->mMask) return;

	if (this->mMask != nullptr) {
		QObject::disconnect(this->mMask, nullptr, this, nullptr);
	}

	this->mMask = mask;

	if (mask != nullptr) {
		QObject::connect(mask, &QObject::destroyed, this, &ProxyWindowBase::onMaskDestroyed);
		QObject::connect(mask, &PendingRegion::changed, this, &ProxyWindowBase::onMaskChanged);
	}

	this->onMaskChanged();
	emit this->maskChanged();
}

void ProxyWindowBase::setSurfaceFormat(QsSurfaceFormat format) {
	if (format == this->qsSurfaceFormat) return;
	if (this->window != nullptr) {
		qmlWarning(this) << "Cannot set window surface format.";
		return;
	}

	this->qsSurfaceFormat = format;
	emit this->surfaceFormatChanged();
}

qreal ProxyWindowBase::devicePixelRatio() const {
	if (this->window != nullptr) return this->window->devicePixelRatio();
	if (this->mScreen != nullptr) return this->mScreen->devicePixelRatio();
	return 1.0;
}

void ProxyWindowBase::onMaskChanged() {
	if (this->window != nullptr) this->updateMask();
}

void ProxyWindowBase::onMaskDestroyed() {
	this->mMask = nullptr;
	this->onMaskChanged();
	emit this->maskChanged();
}

void ProxyWindowBase::updateMask() {
	this->pendingPolish.inputMask = true;
	this->schedulePolish();
}

QQmlListProperty<QObject> ProxyWindowBase::data() {
	return this->mContentItem->property("data").value<QQmlListProperty<QObject>>();
}

void ProxyWindowBase::onWidthChanged() { this->mContentItem->setWidth(this->width()); }
void ProxyWindowBase::onHeightChanged() { this->mContentItem->setHeight(this->height()); }

QPointF ProxyWindowBase::itemPosition(QQuickItem* item) const {
	if (!item) {
		qCritical() << "Cannot map position of null item.";
		return {};
	}

	return this->mContentItem->mapFromItem(item, 0, 0);
}

QRectF ProxyWindowBase::itemRect(QQuickItem* item) const {
	if (!item) {
		qCritical() << "Cannot map position of null item.";
		return {};
	}

	return this->mContentItem->mapFromItem(item, item->boundingRect());
}

QPointF ProxyWindowBase::mapFromItem(QQuickItem* item, QPointF point) const {
	if (!item) {
		qCritical() << "Cannot map position of null item.";
		return {};
	}

	return this->mContentItem->mapFromItem(item, point);
}

QPointF ProxyWindowBase::mapFromItem(QQuickItem* item, qreal x, qreal y) const {
	if (!item) {
		qCritical() << "Cannot map position of null item.";
		return {};
	}

	return this->mContentItem->mapFromItem(item, x, y);
}

QRectF ProxyWindowBase::mapFromItem(QQuickItem* item, QRectF rect) const {
	if (!item) {
		qCritical() << "Cannot map position of null item.";
		return {};
	}

	return this->mContentItem->mapFromItem(item, rect);
}

QRectF
ProxyWindowBase::mapFromItem(QQuickItem* item, qreal x, qreal y, qreal width, qreal height) const {
	if (!item) {
		qCritical() << "Cannot map position of null item.";
		return {};
	}

	return this->mContentItem->mapFromItem(item, x, y, width, height);
}

ProxyWindowAttached::ProxyWindowAttached(QQuickItem* parent): QsWindowAttached(parent) {
	this->updateWindow();
}

QObject* ProxyWindowAttached::window() const { return this->mWindowInterface; }
ProxyWindowBase* ProxyWindowAttached::proxyWindow() const { return this->mWindow; }

QQuickItem* ProxyWindowAttached::contentItem() const {
	return this->mWindow ? this->mWindow->contentItem() : nullptr;
}

void ProxyWindowAttached::updateWindow() {
	auto* window = static_cast<QQuickItem*>(this->parent())->window(); // NOLINT

	if (auto* proxy = qobject_cast<ProxiedWindow*>(window)) {
		this->setWindow(proxy->proxy());
	} else {
		this->setWindow(nullptr);
	}
}

void ProxyWindowAttached::setWindow(ProxyWindowBase* window) {
	if (window == this->mWindow) return;
	this->mWindow = window;
	auto* parentInterface = window ? qobject_cast<WindowInterface*>(window->parent()) : nullptr;
	this->mWindowInterface = parentInterface ? static_cast<QObject*>(parentInterface) : window;
	emit this->windowChanged();
}

bool ProxiedWindow::event(QEvent* event) {
	if (event->type() == QEvent::DevicePixelRatioChange) {
		emit this->devicePixelRatioChanged();
	}

	return this->QQuickWindow::event(event);
}

void ProxiedWindow::exposeEvent(QExposeEvent* event) {
	this->QQuickWindow::exposeEvent(event);
	emit this->exposed();
}

void ProxyWindowContentItem::updatePolish() { emit this->polished(); }

void ProxyWindowBase::onPolished() {
	if (this->pendingPolish.inputMask) {
		QRegion mask;
		if (this->mMask != nullptr) {
			mask = this->mMask->applyTo(QRect(0, 0, this->width(), this->height()));
		}

		this->window->setFlag(Qt::WindowTransparentForInput, this->mMask != nullptr && mask.isEmpty());
		this->window->setMask(mask);

		this->pendingPolish.inputMask = false;
	}

	emit this->polished();
}
