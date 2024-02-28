#include "session_lock.hpp"

#include <private/qwaylanddisplay_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qwindow.h>

#include "lock.hpp"
#include "manager.hpp"
#include "shell_integration.hpp"
#include "surface.hpp"

static QSWaylandSessionLockManager* manager() {
	static QSWaylandSessionLockManager* manager = nullptr;

	if (manager == nullptr) {
		manager = new QSWaylandSessionLockManager();
	}

	return manager;
}

bool SessionLockManager::lock() {
	if (SessionLockManager::sessionLocked()) return false;
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
	auto* lock = this->mLock;
	this->mLock = nullptr;
	delete lock;
	return true;
}

bool SessionLockManager::isLocked() const { return this->mLock != nullptr; }
bool SessionLockManager::sessionLocked() { return manager()->isLocked(); }

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

bool LockWindowExtension::attach(QWindow* window, SessionLockManager* manager) {
	if (this->surface != nullptr) throw "Cannot change the attached window of a LockWindowExtension";

	auto* current = LockWindowExtension::get(window);
	QtWaylandClient::QWaylandWindow* waylandWindow = nullptr;

	if (current != nullptr) {
		current->surface->setExtension(this);
	} else {
		window->create();

		waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
		if (waylandWindow == nullptr) {
			qWarning() << window << "is not a wayland window. Cannot create lock surface.";
			return false;
		}

		static QSWaylandSessionLockIntegration* lockIntegration = nullptr;
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
	if (this->surface == nullptr) immediatelyVisible = true;
	else this->surface->setVisible();
}
