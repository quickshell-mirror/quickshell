#include "session_lock.hpp"

#include <qcolor.h>
#include <qcoreapplication.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/qmlscreen.hpp"
#include "../core/reload.hpp"
#include "session_lock/session_lock.hpp"

void SessionLock::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<SessionLock*>(oldInstance);

	if (old != nullptr) {
		QObject::disconnect(old->manager, nullptr, old, nullptr);
		this->manager = old->manager;
		this->manager->setParent(this);
	} else {
		this->manager = new SessionLockManager(this);
	}

	// clang-format off
	QObject::connect(this->manager, &SessionLockManager::locked, this, &SessionLock::secureStateChanged);
	QObject::connect(this->manager, &SessionLockManager::unlocked, this, &SessionLock::secureStateChanged);

	QObject::connect(this->manager, &SessionLockManager::unlocked, this, &SessionLock::unlock);

	auto* app = QCoreApplication::instance();
	auto* guiApp = qobject_cast<QGuiApplication*>(app);

	if (guiApp != nullptr) {
		QObject::connect(guiApp, &QGuiApplication::primaryScreenChanged, this, &SessionLock::onScreensChanged);
		QObject::connect(guiApp, &QGuiApplication::screenAdded, this, &SessionLock::onScreensChanged);
		QObject::connect(guiApp, &QGuiApplication::screenRemoved, this, &SessionLock::onScreensChanged);
	}
	// clang-format on

	if (this->lockTarget) {
		if (!this->manager->lock()) this->lockTarget = false;
		this->updateSurfaces(old);
	} else {
		this->setLocked(false);
	}
}

void SessionLock::updateSurfaces(SessionLock* old) {
	if (this->manager->isLocked()) {
		auto screens = QGuiApplication::screens();

		auto map = this->surfaces.toStdMap();
		for (auto& [screen, surface]: map) {
			if (!screens.contains(screen)) {
				this->surfaces.remove(screen);
				surface->deleteLater();
			}
		}

		if (this->mSurfaceComponent == nullptr) {
			qWarning() << "SessionLock.surface is null. Aborting lock.";
			this->unlock();
			return;
		}

		for (auto* screen: screens) {
			if (!this->surfaces.contains(screen)) {
				auto* instanceObj =
				    this->mSurfaceComponent->create(QQmlEngine::contextForObject(this->mSurfaceComponent));
				auto* instance = qobject_cast<SessionLockSurface*>(instanceObj);

				if (instance == nullptr) {
					qWarning() << "SessionLock.surface does not create a SessionLockSurface. Aborting lock.";
					if (instanceObj != nullptr) instanceObj->deleteLater();
					this->unlock();
					return;
				}

				instance->setParent(this);
				instance->setScreen(screen);

				auto* oldInstance = old == nullptr ? nullptr : old->surfaces.value(screen, nullptr);
				instance->onReload(oldInstance);

				this->surfaces[screen] = instance;
			}

			for (auto* surface: this->surfaces.values()) {
				surface->show();
			}
		}
	}
}

void SessionLock::unlock() {
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

void SessionLock::onScreensChanged() { this->updateSurfaces(); }

bool SessionLock::isLocked() const {
	return this->manager == nullptr ? this->lockTarget : this->manager->isLocked();
}

bool SessionLock::isSecure() const {
	return this->manager != nullptr && SessionLockManager::isSecure();
}

void SessionLock::setLocked(bool locked) {
	if (this->isLocked() == locked) return;
	this->lockTarget = locked;

	if (this->manager == nullptr) {
		emit this->lockStateChanged();
		return;
	}

	if (locked) {
		if (!this->manager->lock()) this->lockTarget = false;
		this->updateSurfaces();
		if (this->lockTarget) emit this->lockStateChanged();
	} else {
		this->unlock(); // emits lockStateChanged
	}
}

QQmlComponent* SessionLock::surfaceComponent() const { return this->mSurfaceComponent; }

void SessionLock::setSurfaceComponent(QQmlComponent* surfaceComponent) {
	if (this->mSurfaceComponent != nullptr) this->mSurfaceComponent->deleteLater();
	if (surfaceComponent != nullptr) surfaceComponent->setParent(this);

	this->mSurfaceComponent = surfaceComponent;
	emit this->surfaceComponentChanged();
}

SessionLockSurface::SessionLockSurface(QObject* parent)
    : Reloadable(parent)
    , mContentItem(new QQuickItem())
    , ext(new LockWindowExtension(this)) {
	this->mContentItem->setParent(this);

	// clang-format off
	QObject::connect(this, &SessionLockSurface::widthChanged, this, &SessionLockSurface::onWidthChanged);
	QObject::connect(this, &SessionLockSurface::heightChanged, this, &SessionLockSurface::onHeightChanged);
	// clang-format on
}

SessionLockSurface::~SessionLockSurface() {
	if (this->window != nullptr) {
		this->window->deleteLater();
	}
}

void SessionLockSurface::onReload(QObject* oldInstance) {
	if (auto* old = qobject_cast<SessionLockSurface*>(oldInstance)) {
		this->window = old->disownWindow();
	}

	if (this->window == nullptr) {
		this->window = new QQuickWindow();
	}

	this->mContentItem->setParentItem(this->window->contentItem());

	this->mContentItem->setWidth(this->width());
	this->mContentItem->setHeight(this->height());

	if (this->mScreen != nullptr) this->window->setScreen(this->mScreen);
	this->window->setColor(this->mColor);

	// clang-format off
	QObject::connect(this->window, &QWindow::visibilityChanged, this, &SessionLockSurface::visibleChanged);
	QObject::connect(this->window, &QWindow::widthChanged, this, &SessionLockSurface::widthChanged);
	QObject::connect(this->window, &QWindow::heightChanged, this, &SessionLockSurface::heightChanged);
	QObject::connect(this->window, &QWindow::screenChanged, this, &SessionLockSurface::screenChanged);
	QObject::connect(this->window, &QQuickWindow::colorChanged, this, &SessionLockSurface::colorChanged);
	// clang-format on

	if (auto* parent = qobject_cast<SessionLock*>(this->parent())) {
		if (!this->ext->attach(this->window, parent->manager)) {
			qWarning(
			) << "Failed to attach LockWindowExtension to window. Surface will not behave correctly.";
		}
	} else {
		qWarning(
		) << "SessionLockSurface parent is not a SessionLock. Surface will not behave correctly.";
	}
}

QQuickWindow* SessionLockSurface::disownWindow() {
	QObject::disconnect(this->window, nullptr, this, nullptr);
	this->mContentItem->setParentItem(nullptr);

	auto* window = this->window;
	this->window = nullptr;
	return window;
}

void SessionLockSurface::show() { this->ext->setVisible(); }

QQuickItem* SessionLockSurface::contentItem() const { return this->mContentItem; }

bool SessionLockSurface::isVisible() const { return this->window->isVisible(); }

qint32 SessionLockSurface::width() const {
	if (this->window == nullptr) return 0;
	else return this->window->width();
}

qint32 SessionLockSurface::height() const {
	if (this->window == nullptr) return 0;
	else return this->window->height();
}

QuickshellScreenInfo* SessionLockSurface::screen() const {
	QScreen* qscreen = nullptr;

	if (this->window == nullptr) {
		if (this->mScreen != nullptr) qscreen = this->mScreen;
	} else {
		qscreen = this->window->screen();
	}

	return new QuickshellScreenInfo(
	    const_cast<SessionLockSurface*>(this), // NOLINT
	    qscreen
	);
}

void SessionLockSurface::setScreen(QScreen* qscreen) {
	if (this->mScreen != nullptr) {
		QObject::disconnect(this->mScreen, nullptr, this, nullptr);
	}

	if (qscreen != nullptr) {
		QObject::connect(qscreen, &QObject::destroyed, this, &SessionLockSurface::onScreenDestroyed);
	}

	if (this->window == nullptr) {
		this->mScreen = qscreen;
		emit this->screenChanged();
	} else this->window->setScreen(qscreen);
}

void SessionLockSurface::onScreenDestroyed() { this->mScreen = nullptr; }

QColor SessionLockSurface::color() const {
	if (this->window == nullptr) return this->mColor;
	else return this->window->color();
}

void SessionLockSurface::setColor(QColor color) {
	if (this->window == nullptr) {
		this->mColor = color;
		emit this->colorChanged();
	} else this->window->setColor(color);
}

QQmlListProperty<QObject> SessionLockSurface::data() {
	return this->mContentItem->property("data").value<QQmlListProperty<QObject>>();
}

void SessionLockSurface::onWidthChanged() { this->mContentItem->setWidth(this->width()); }
void SessionLockSurface::onHeightChanged() { this->mContentItem->setHeight(this->height()); }
