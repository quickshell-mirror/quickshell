#include "session_lock.hpp"

#include <private/qwaylanddisplay_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qwindow.h>

#include "lock.hpp"
#include "manager.hpp"
#include "shell_integration.hpp"
#include "surface.hpp"

namespace {
QSWaylandSessionLockManager* manager() {
	static QSWaylandSessionLockManager* manager = nullptr; // NOLINT

	if (manager == nullptr) {
		manager = new QSWaylandSessionLockManager();
	}

	return manager;
}
} // namespace

bool SessionLockManager::lockAvailable() { return manager()->isActive(); }

bool SessionLockManager::lock() {
	if (this->isLocked() || SessionLockManager::sessionLocked()) return false;
	this->mLock = manager()->acquireLock();
	this->mLock->setParent(this);

	// clang-format off
	QObject::connect(this->mLock, &QSWaylandSessionLock::compositorLocked, this, &SessionLockManager::locked);
	QObject::connect(this->mLock, &QSWaylandSessionLock::unlocked, this, &SessionLockManager::unlocked);
	// clang-format on
	return true;
}

bool SessionLockManager::unlock() {
	if (!this->isLocked()) return false;
	this->mLock->unlock();
	auto* lock = this->mLock;
	this->mLock = nullptr;
	delete lock;
	return true;
}

bool SessionLockManager::isLocked() const { return this->mLock != nullptr; }
bool SessionLockManager::sessionLocked() { return manager()->isLocked(); }
bool SessionLockManager::isSecure() { return manager()->isSecure(); }

LockWindowExtension::~LockWindowExtension() {
	if (this->surface != nullptr) {
		this->surface->setExtension(nullptr);
	}
}

LockWindowExtension* LockWindowExtension::get(QWindow* window) {
	auto v = window->property("sessionlock_ext");

	if (v.canConvert<LockWindowExtension*>()) {
		return v.value<LockWindowExtension*>();
	} else {
		return nullptr;
	}
}

bool LockWindowExtension::isAttached() const { return this->surface != nullptr; }

bool LockWindowExtension::attach(QWindow* window, SessionLockManager* manager) {
	if (this->surface != nullptr)
		qFatal() << "Cannot change the attached window of a LockWindowExtension";

	auto* current = LockWindowExtension::get(window);
	QtWaylandClient::QWaylandWindow* waylandWindow = nullptr;

	if (current != nullptr) {
		current->surface->setExtension(this);
	} else {
		// Qt appears to be resetting the window's screen on creation on some systems. This works around it.
		auto* screen = window->screen();
		window->create();
		window->setScreen(screen);

		waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
		if (waylandWindow == nullptr) {
			qWarning() << window << "is not a wayland window. Cannot create lock surface.";
			return false;
		}

		static QSWaylandSessionLockIntegration* lockIntegration = nullptr; // NOLINT
		if (lockIntegration == nullptr) {
			lockIntegration = new QSWaylandSessionLockIntegration();
			if (!lockIntegration->initialize(waylandWindow->display())) {
				delete lockIntegration;
				lockIntegration = nullptr;
				qWarning() << "Failed to initialize lockscreen integration";
			}
		}

		waylandWindow->setShellIntegration(lockIntegration);
	}

	this->setParent(window);
	window->setProperty("sessionlock_ext", QVariant::fromValue(this));
	this->lock = manager->mLock;

	if (waylandWindow != nullptr) {
		this->surface = new QSWaylandSessionLockSurface(waylandWindow);
		if (this->immediatelyVisible) this->surface->setVisible();
	}

	return true;
}

void LockWindowExtension::setVisible() {
	if (this->surface == nullptr) this->immediatelyVisible = true;
	else this->surface->setVisible();
}
