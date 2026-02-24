#include "session_lock.hpp"

#include <private/qwaylandscreen_p.h>
#include <qcolor.h>
#include <qcoreapplication.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qquickgraphicsconfiguration.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/qmlglobal.hpp"
#include "../core/qmlscreen.hpp"
#include "../core/reload.hpp"
#include "session_lock/session_lock.hpp"

void WlSessionLock::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<WlSessionLock*>(oldInstance);

	if (old != nullptr) {
		QObject::disconnect(old->manager, nullptr, old, nullptr);
		this->manager = old->manager;
		this->manager->setParent(this);
	} else {
		this->manager = new SessionLockManager(this);
	}

	// clang-format off
	QObject::connect(this->manager, &SessionLockManager::locked, this, &WlSessionLock::secureStateChanged);
	QObject::connect(this->manager, &SessionLockManager::unlocked, this, &WlSessionLock::secureStateChanged);

	QObject::connect(this->manager, &SessionLockManager::unlocked, this, &WlSessionLock::unlock);

	auto* app = QCoreApplication::instance();
	auto* guiApp = qobject_cast<QGuiApplication*>(app);

	if (guiApp != nullptr) {
		QObject::connect(guiApp, &QGuiApplication::primaryScreenChanged, this, &WlSessionLock::onScreensChanged);
		QObject::connect(guiApp, &QGuiApplication::screenAdded, this, &WlSessionLock::onScreensChanged);
		QObject::connect(guiApp, &QGuiApplication::screenRemoved, this, &WlSessionLock::onScreensChanged);
	}
	// clang-format on

	this->realizeLockTarget(old);
}

void WlSessionLock::updateSurfaces(bool show, WlSessionLock* old) {
	auto screens = QGuiApplication::screens();

	screens.removeIf([](QScreen* screen) {
		if (dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle()) == nullptr) {
			qDebug() << "Not creating lock surface for screen" << screen
			         << "as it is not backed by a wayland screen.";

			return true;
		}

		return false;
	});

	auto map = this->surfaces.toStdMap();
	for (auto& [screen, surface]: map) {
		if (!screens.contains(screen)) {
			this->surfaces.remove(screen);
			surface->deleteLater();
		}
	}

	for (auto* screen: screens) {
		if (!this->surfaces.contains(screen)) {
			auto* instanceObj =
			    this->mSurfaceComponent->create(QQmlEngine::contextForObject(this->mSurfaceComponent));
			auto* instance = qobject_cast<WlSessionLockSurface*>(instanceObj);

			if (instance == nullptr) {
				qWarning()
				    << "WlSessionLock.surface does not create a WlSessionLockSurface. Aborting lock.";
				if (instanceObj != nullptr) instanceObj->deleteLater();
				this->unlock();
				return;
			}

			instance->setParent(this);
			instance->setScreen(screen);

			auto* oldInstance = old == nullptr ? nullptr : old->surfaces.value(screen, nullptr);
			instance->reload(oldInstance);

			this->surfaces[screen] = instance;
		}
	}

	if (show) {
		if (!this->manager->isLocked()) {
			qFatal() << "Tried to show lockscreen surfaces without active lock";
		}

		for (auto* surface: this->surfaces.values()) {
			surface->show();
		}
	}
}

void WlSessionLock::realizeLockTarget(WlSessionLock* old) {
	if (this->lockTarget) {
		if (!SessionLockManager::lockAvailable()) {
			qCritical() << "Cannot start session lock: The current compositor does not support the "
			               "ext-session-lock-v1 protocol.";
			this->unlock();
			return;
		}

		if (this->mSurfaceComponent == nullptr) {
			qWarning() << "WlSessionLock.surface is null. Aborting lock.";
			this->unlock();
			return;
		}

		// preload initial surfaces to make the chance of the compositor displaying a blank
		// frame before the lock surfaces are shown as low as possible.
		this->updateSurfaces(false);

		if (!this->manager->lock()) this->lockTarget = false;

		this->updateSurfaces(true, old);
	} else {
		this->unlock(); // emits lockStateChanged
	}
}

void WlSessionLock::unlock() {
	if (this->isLocked()) {
		this->lockTarget = false;
		this->manager->unlock();

		for (auto* surface: this->surfaces) {
			surface->deleteLater();
		}

		this->surfaces.clear();

		emit this->lockStateChanged();
	}
}

void WlSessionLock::onScreensChanged() {
	if (this->manager != nullptr && this->manager->isLocked()) {
		this->updateSurfaces(true);
	}
}

bool WlSessionLock::isLocked() const {
	return this->manager == nullptr ? this->lockTarget : this->manager->isLocked();
}

bool WlSessionLock::isSecure() const {
	return this->manager != nullptr && SessionLockManager::isSecure();
}

void WlSessionLock::setLocked(bool locked) {
	if (this->isLocked() == locked) return;
	this->lockTarget = locked;

	if (this->manager == nullptr) {
		emit this->lockStateChanged();
		return;
	}

	this->realizeLockTarget();
}

QQmlComponent* WlSessionLock::surfaceComponent() const { return this->mSurfaceComponent; }

void WlSessionLock::setSurfaceComponent(QQmlComponent* surfaceComponent) {
	if (this->manager != nullptr && this->manager->isLocked()) {
		qCritical() << "WlSessionLock.surfaceComponent cannot be changed while the lock is active.";
		return;
	}

	if (this->mSurfaceComponent != nullptr) this->mSurfaceComponent->deleteLater();
	if (surfaceComponent != nullptr) surfaceComponent->setParent(this);

	this->mSurfaceComponent = surfaceComponent;
	emit this->surfaceComponentChanged();
}

WlSessionLockSurface::WlSessionLockSurface(QObject* parent)
    : Reloadable(parent)
    , mContentItem(new QQuickItem())
    , ext(new LockWindowExtension(this)) {
	QQmlEngine::setObjectOwnership(this->mContentItem, QQmlEngine::CppOwnership);
	this->mContentItem->setParent(this);

	// clang-format off
	QObject::connect(this, &WlSessionLockSurface::widthChanged, this, &WlSessionLockSurface::onWidthChanged);
	QObject::connect(this, &WlSessionLockSurface::heightChanged, this, &WlSessionLockSurface::onHeightChanged);
	// clang-format on
}

WlSessionLockSurface::~WlSessionLockSurface() {
	if (this->window != nullptr) {
		this->window->deleteLater();
	}
}

void WlSessionLockSurface::onReload(QObject* oldInstance) {
	if (auto* old = qobject_cast<WlSessionLockSurface*>(oldInstance)) {
		this->window = old->disownWindow();
	}

	if (this->window == nullptr) {
		this->window = new QQuickWindow();

		// needed for vulkan dmabuf import, qt ignores these if not applicable
		auto graphicsConfig = this->window->graphicsConfiguration();
		graphicsConfig.setDeviceExtensions({
		    "VK_KHR_external_memory_fd",
		    "VK_EXT_external_memory_dma_buf",
		    "VK_EXT_image_drm_format_modifier",
		});
		this->window->setGraphicsConfiguration(graphicsConfig);
	}

	this->mContentItem->setParentItem(this->window->contentItem());

	this->mContentItem->setWidth(this->width());
	this->mContentItem->setHeight(this->height());

	if (this->mScreen != nullptr) this->window->setScreen(this->mScreen);
	this->window->setColor(this->mColor);

	// clang-format off
	QObject::connect(this->window, &QWindow::visibilityChanged, this, &WlSessionLockSurface::visibleChanged);
	QObject::connect(this->window, &QWindow::widthChanged, this, &WlSessionLockSurface::widthChanged);
	QObject::connect(this->window, &QWindow::heightChanged, this, &WlSessionLockSurface::heightChanged);
	QObject::connect(this->window, &QWindow::screenChanged, this, &WlSessionLockSurface::screenChanged);
	QObject::connect(this->window, &QQuickWindow::colorChanged, this, &WlSessionLockSurface::colorChanged);
	// clang-format on
}

void WlSessionLockSurface::attach() {
	if (this->ext->isAttached()) return;

	if (auto* parent = qobject_cast<WlSessionLock*>(this->parent())) {
		if (!this->ext->attach(this->window, parent->manager)) {
			qFatal() << "Failed to attach WlSessionLockSurface";
		}
	} else {
		qFatal() << "Tried to attach a WlSessionLockSurface whose parent is not a WlSessionLock";
	}
}

QQuickWindow* WlSessionLockSurface::disownWindow() {
	QObject::disconnect(this->window, nullptr, this, nullptr);
	this->mContentItem->setParentItem(nullptr);

	auto* window = this->window;
	this->window = nullptr;
	return window;
}

void WlSessionLockSurface::show() {
	this->attach();
	this->ext->setVisible();
}

QQuickItem* WlSessionLockSurface::contentItem() const { return this->mContentItem; }

bool WlSessionLockSurface::isVisible() const { return this->window->isVisible(); }

qint32 WlSessionLockSurface::width() const {
	if (this->window == nullptr) return 0;
	else return this->window->width();
}

qint32 WlSessionLockSurface::height() const {
	if (this->window == nullptr) return 0;
	else return this->window->height();
}

QuickshellScreenInfo* WlSessionLockSurface::screen() const {
	QScreen* qscreen = nullptr;

	if (this->window == nullptr) {
		if (this->mScreen != nullptr) qscreen = this->mScreen;
	} else {
		qscreen = this->window->screen();
	}

	return QuickshellTracked::instance()->screenInfo(qscreen);
}

void WlSessionLockSurface::setScreen(QScreen* qscreen) {
	if (this->mScreen != nullptr) {
		QObject::disconnect(this->mScreen, nullptr, this, nullptr);
	}

	if (qscreen != nullptr) {
		QObject::connect(qscreen, &QObject::destroyed, this, &WlSessionLockSurface::onScreenDestroyed);
	}

	if (this->window == nullptr) {
		this->mScreen = qscreen;
	} else {
		this->window->setScreen(qscreen);
	}

	emit this->screenChanged();
}

void WlSessionLockSurface::onScreenDestroyed() { this->mScreen = nullptr; }

QColor WlSessionLockSurface::color() const {
	if (this->window == nullptr) return this->mColor;
	else return this->window->color();
}

void WlSessionLockSurface::setColor(QColor color) {
	if (this->window == nullptr) {
		this->mColor = color;
		emit this->colorChanged();
	} else this->window->setColor(color);
}

QQmlListProperty<QObject> WlSessionLockSurface::data() {
	return this->mContentItem->property("data").value<QQmlListProperty<QObject>>();
}

void WlSessionLockSurface::onWidthChanged() { this->mContentItem->setWidth(this->width()); }
void WlSessionLockSurface::onHeightChanged() { this->mContentItem->setHeight(this->height()); }
