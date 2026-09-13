#include "session_lock.hpp"

#include <qcoreapplication.h>
#include <qpointer.h>
#include <qquickwindow.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include "../session_lock.hpp"

void TestSessionLockSurface::errorSchedulesRetry() {
	auto surface = WlSessionLockSurface();
	surface.reload(nullptr);
	auto* window = surface.contentItem()->window();

	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "test initialization failure");
	QVERIFY(surface.renderRetryTimer.isActive());
	QCOMPARE(surface.renderRetryTimer.interval(), 1000);
	QCOMPARE(surface.contentItem()->window(), window);
}

void TestSessionLockSurface::reloadTransfersPendingRetry() {
	auto old = WlSessionLockSurface();
	old.reload(nullptr);
	auto* window = old.contentItem()->window();
	old.renderRetryTimer.setInterval(8000);
	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "test initialization failure");

	// Queue a frame notification before transferring ownership.
	emit window->frameSwapped();
	auto surface = WlSessionLockSurface();
	surface.reload(&old);
	QCoreApplication::processEvents();
	QVERIFY(!old.renderRetryTimer.isActive());
	QVERIFY(surface.renderRetryTimer.isActive());
	QCOMPARE(surface.renderRetryTimer.interval(), 8000);
	QCOMPARE(surface.contentItem()->window(), window);

	// A new frame notification belongs to the new owner only.
	emit window->frameSwapped();
	QTRY_VERIFY(!surface.renderRetryTimer.isActive());
	QCOMPARE(surface.renderRetryTimer.interval(), 1000);
	QCOMPARE(old.renderRetryTimer.interval(), 8000);
}

void TestSessionLockSurface::frameResetsRetry() {
	auto surface = WlSessionLockSurface();
	surface.reload(nullptr);
	auto* window = surface.contentItem()->window();
	surface.renderRetryTimer.setInterval(30000);
	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "test initialization failure");
	emit window->frameSwapped();
	QTRY_VERIFY(!surface.renderRetryTimer.isActive());
	QCOMPARE(surface.renderRetryTimer.interval(), 1000);
}

void TestSessionLockSurface::unlockedSurfaceDoesNotRetry() {
	auto lock = WlSessionLock();
	auto surface = WlSessionLockSurface(&lock);
	surface.reload(nullptr);
	auto window = QPointer(surface.contentItem()->window());
	surface.renderRetryTimer.setInterval(1);
	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "test initialization failure");
	QTRY_VERIFY(!surface.renderRetryTimer.isActive());
	QVERIFY(window);
	QCOMPARE(surface.contentItem()->window(), window.data());
}

QTEST_MAIN(TestSessionLockSurface);
