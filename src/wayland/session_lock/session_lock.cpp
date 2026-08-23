#include "session_lock.hpp"

#include <private/qwaylanddisplay_p.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qdbusextratypes.h>
#include <qdbusmessage.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "../../core/logcat.hpp"
#include "lock.hpp"
#include "manager.hpp"
#include "shell_integration.hpp"
#include "surface.hpp"

namespace {
QS_LOGGING_CATEGORY(logSessionLock, "quickshell.wayland.sessionlock", QtWarningMsg);

QSWaylandSessionLockManager* manager() {
	static QSWaylandSessionLockManager* manager = nullptr; // NOLINT

	if (manager == nullptr) {
		manager = new QSWaylandSessionLockManager();
	}

	return manager;
}

} // namespace

SessionLockManager::SessionLockManager(bool logindLockedHint, QObject* parent)
    : QObject(parent)
    , mLogindLockedHint(logindLockedHint) {}

bool SessionLockManager::lockAvailable() { return manager()->isActive(); }

bool SessionLockManager::lock() {
	if (this->isLocked() || SessionLockManager::sessionLocked()) return false;

	this->mLock = manager()->acquireLock();
	this->mLock->setParent(this);

	// clang-format off
	QObject::connect(this->mLock, &QSWaylandSessionLock::compositorLocked, this, [this]() {
		this->updateLogindLockedHint(true);
		emit this->locked();
	});

	QObject::connect(this->mLock, &QSWaylandSessionLock::unlocked, this, [this]() {
		this->updateLogindLockedHint(false);
		emit this->unlocked();
	});
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

void SessionLockManager::updateLogindLockedHint(bool locked) {
	if (!this->mLogindLockedHint) return;

	auto generation = ++this->mLogindLockedHintGeneration;

	if (!this->mLogindSessionPath.isEmpty()) {
		this->updateLogindLockedHint(this->mLogindSessionPath, locked, generation);
		return;
	}

	auto sessionId = qEnvironmentVariable("XDG_SESSION_ID");
	if (sessionId.isEmpty()) {
		qCWarning(logSessionLock) << "Cannot set logind LockedHint: XDG_SESSION_ID is unset.";
		return;
	}

	auto bus = QDBusConnection::systemBus();
	if (!bus.isConnected()) {
		qCWarning(logSessionLock) << "Cannot set logind LockedHint: system bus is not connected.";
		return;
	}

	auto message = QDBusMessage::createMethodCall(
	    "org.freedesktop.login1",
	    "/org/freedesktop/login1",
	    "org.freedesktop.login1.Manager",
	    "GetSession"
	);

	message << sessionId;

	auto* call = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
	QObject::connect(
	    call,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this, locked, generation](QDBusPendingCallWatcher* call) {
		    const QDBusPendingReply<QDBusObjectPath> reply = *call;
		    if (reply.isError()) {
			    qCWarning(logSessionLock)
			        << "Failed to resolve logind session:" << reply.error().message();
		    } else if (generation == this->mLogindLockedHintGeneration) {
			    this->mLogindSessionPath = reply.value().path();
			    this->updateLogindLockedHint(this->mLogindSessionPath, locked, generation);
		    }

		    delete call;
	    }
	);
}

void SessionLockManager::updateLogindLockedHint(
    const QString& sessionPath,
    bool locked,
    quint64 generation
) {
	if (generation != this->mLogindLockedHintGeneration) return;

	auto message = QDBusMessage::createMethodCall(
	    "org.freedesktop.login1",
	    sessionPath,
	    "org.freedesktop.login1.Session",
	    "SetLockedHint"
	);

	message << locked;

	auto* call = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message), this);
	QObject::connect(
	    call,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [locked](QDBusPendingCallWatcher* call) {
		    const QDBusPendingReply<> reply = *call;
		    if (reply.isError()) {
			    qCWarning(logSessionLock)
			        << "Failed to set logind LockedHint to" << locked << ':' << reply.error().message();
		    } else {
			    qCDebug(logSessionLock) << "Set logind LockedHint to" << locked;
		    }

		    delete call;
	    }
	);
}

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
