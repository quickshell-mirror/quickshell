#include "session_lock.hpp"

#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qguiapplication.h>
#include <qpointer.h>
#include <qqml.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qquickwindow.h>
#include <qscopeguard.h>
#include <qsignalspy.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include "../session_lock.hpp"
#include "../session_lock/session_lock.hpp"

void TestSessionLock::initialGraphicsError() {
	WlSessionLockSurface surface;
	surface.reload(nullptr);
	surface.handleInitialGraphicsErrors();
	auto failed = QSignalSpy(&surface, &WlSessionLockSurface::graphicsInitializationFailed);

	emit surface.contentItem()->window()->sceneGraphError(
	    QQuickWindow::ContextNotAvailable,
	    "test initialization failure"
	);
	QCOMPARE(failed.count(), 1);
	QCOMPARE(failed.first().at(1).toString(), "test initialization failure");
}

void TestSessionLock::initializedSurfaceDoesNotAbort() {
	WlSessionLockSurface surface;
	surface.reload(nullptr);
	surface.handleInitialGraphicsErrors();
	auto failed = QSignalSpy(&surface, &WlSessionLockSurface::graphicsInitializationFailed);
	auto* window = surface.contentItem()->window();

	emit window->sceneGraphInitialized();
	QCoreApplication::sendPostedEvents(&surface, QEvent::MetaCall);
	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "later failure");
	QCOMPARE(failed.count(), 0);
}

void TestSessionLock::disownedWindowDoesNotAbort() {
	WlSessionLockSurface old;
	old.reload(nullptr);
	old.handleInitialGraphicsErrors();
	auto failed = QSignalSpy(&old, &WlSessionLockSurface::graphicsInitializationFailed);
	WlSessionLockSurface surface;
	surface.reload(&old);

	emit surface.contentItem()->window()->sceneGraphError(
	    QQuickWindow::ContextNotAvailable,
	    "failure after ownership transfer"
	);
	QCOMPARE(failed.count(), 0);
}

namespace {
bool disposableSession() {
	return qEnvironmentVariableIsSet("QS_TEST_SESSION_LOCK")
	    && QGuiApplication::platformName() == "wayland";
}

QQmlComponent* lockComponent(QQmlEngine* engine) {
	qmlRegisterType<WlSessionLockSurface>("SessionLockTest", 1, 0, "LockSurface");
	auto* component = new QQmlComponent(engine);
	component->setData("import SessionLockTest; LockSurface {}", QUrl());
	return component;
}
} // namespace

void TestSessionLock::failedLockUnlocks() {
	if (!disposableSession()) QSKIP("Requires an explicitly enabled disposable Wayland session");

	QQmlEngine engine;
	WlSessionLock lock;
	auto unlock = qScopeGuard([&]() {
		lock.setLocked(false);
		QGuiApplication::sync();
	});
	auto* component = lockComponent(&engine);
	QVERIFY2(component->isReady(), qPrintable(component->errorString()));
	lock.setSurfaceComponent(component);
	lock.reload(nullptr);
	auto state = QSignalSpy(&lock, &WlSessionLock::lockStateChanged);
	lock.setLocked(true);
	QVERIFY(lock.isLocked());
	QCOMPARE(state.count(), 1);

	auto* surface = lock.findChild<WlSessionLockSurface*>();
	QVERIFY(surface);
	auto window = QPointer(surface->contentItem()->window());
	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "test initialization failure");
	QVERIFY(lock.isLocked());
	QVERIFY(window);
	QCoreApplication::sendPostedEvents(&lock, QEvent::MetaCall);
	QTRY_VERIFY(!lock.isLocked());
	QCOMPARE(state.count(), 2);
	QVERIFY(!lock.isSecure());
	QTRY_VERIFY(window.isNull());

	// A failed attempt must not prevent a later successful lock.
	lock.setLocked(true);
	QTRY_VERIFY(lock.isSecure());
	QVERIFY(lock.isLocked());
}

void TestSessionLock::staleFailureDoesNotUnlock() {
	if (!disposableSession()) QSKIP("Requires an explicitly enabled disposable Wayland session");

	QQmlEngine engine;
	WlSessionLock lock;
	auto unlock = qScopeGuard([&]() {
		lock.setLocked(false);
		QGuiApplication::sync();
	});
	lock.setSurfaceComponent(lockComponent(&engine));
	lock.reload(nullptr);
	lock.setLocked(true);
	QTRY_VERIFY(lock.isSecure());
	auto* surface = lock.findChild<WlSessionLockSurface*>();
	QVERIFY(surface);
	emit surface->graphicsInitializationFailed(QQuickWindow::ContextNotAvailable, "stale failure");
	lock.setLocked(false);
	lock.setLocked(true);
	QCoreApplication::sendPostedEvents(&lock, QEvent::MetaCall);
	QTRY_VERIFY(lock.isSecure());
	QVERIFY(lock.isLocked());
}

void TestSessionLock::transferredManagerIgnoresFailure_data() {
	QTest::addColumn<bool>("pending");
	QTest::newRow("queued failure") << false;
	QTest::newRow("pending acknowledgment") << true;
}

void TestSessionLock::transferredManagerIgnoresFailure() {
	if (!disposableSession()) QSKIP("Requires an explicitly enabled disposable Wayland session");
	QFETCH(const bool, pending);

	QQmlEngine engine;
	WlSessionLock lock;
	QObject owner;
	lock.setSurfaceComponent(lockComponent(&engine));
	lock.reload(nullptr);
	auto* manager = lock.findChild<SessionLockManager*>();
	QVERIFY(manager);
	auto unlock = qScopeGuard([&]() {
		manager->setParent(&lock);
		lock.setLocked(false);
		QGuiApplication::sync();
	});
	auto acknowledged = QSignalSpy(manager, &SessionLockManager::locked);
	lock.setLocked(true);
	if (pending && lock.isSecure()) QSKIP("Compositor already acknowledged the lock");
	if (!pending) QTRY_VERIFY(lock.isSecure());

	auto* surface = lock.findChild<WlSessionLockSurface*>();
	QVERIFY(surface);
	emit surface->graphicsInitializationFailed(
	    QQuickWindow::ContextNotAvailable,
	    "old owner failure"
	);
	if (pending) {
		QCoreApplication::sendPostedEvents(&lock, QEvent::MetaCall);
	}

	// Reload reparents the manager before deleting the old lock and its surfaces.
	manager->setParent(&owner);
	QCoreApplication::sendPostedEvents(&lock, QEvent::MetaCall);
	QTRY_COMPARE(acknowledged.count(), 1);
	QVERIFY(lock.isLocked());
	QVERIFY(lock.isSecure());
}

void TestSessionLock::normalUnlockNotifies() {
	if (!disposableSession()) QSKIP("Requires an explicitly enabled disposable Wayland session");

	QQmlEngine engine;
	WlSessionLock lock;
	auto unlock = qScopeGuard([&]() {
		lock.setLocked(false);
		QGuiApplication::sync();
	});
	lock.setSurfaceComponent(lockComponent(&engine));
	lock.reload(nullptr);
	auto state = QSignalSpy(&lock, &WlSessionLock::lockStateChanged);
	lock.setLocked(true);
	QTRY_VERIFY(lock.isSecure());
	lock.setLocked(false);
	QVERIFY(!lock.isLocked());
	QCOMPARE(state.count(), 2);
	lock.setLocked(false);
	QCOMPARE(state.count(), 2);
}

QTEST_MAIN(TestSessionLock);
