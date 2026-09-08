#include "session_lock.hpp"

#include <qcoreapplication.h>
#include <qnamespace.h>
#include <qpointer.h>
#include <qqml.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qquickwindow.h>
#include <qscopedpointer.h>
#include <qscopeguard.h>
#include <qsignalspy.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
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

void TestSessionLockSurface::frameConnectionIsSingleShot() {
	auto surface = WlSessionLockSurface();
	surface.reload(nullptr);
	auto* window = surface.contentItem()->window();

	emit window->frameSwapped();
	QCoreApplication::processEvents();
	surface.renderRetryTimer.setInterval(8000);
	emit window->frameSwapped();
	QCoreApplication::processEvents();
	QCOMPARE(surface.renderRetryTimer.interval(), 8000);
}

void TestSessionLockSurface::recreationPreservesPropertiesAndFocus() {
	auto surface = WlSessionLockSurface();
	surface.reload(nullptr);
	surface.setColor(Qt::red);
	auto* window = surface.contentItem()->window();
	window->resize(1280, 720);
	window->show();
	QVERIFY(surface.isVisible());

	auto input = QQuickItem(surface.contentItem());
	input.setProperty("authenticationState", "pending");
	input.forceActiveFocus();
	QCOMPARE(window->activeFocusItem(), &input);

	auto width = QSignalSpy(&surface, &WlSessionLockSurface::widthChanged);
	auto height = QSignalSpy(&surface, &WlSessionLockSurface::heightChanged);
	auto visible = QSignalSpy(&surface, &WlSessionLockSurface::visibleChanged);
	auto color = QSignalSpy(&surface, &WlSessionLockSurface::colorChanged);
	auto contentWidth = QSignalSpy(surface.contentItem(), &QQuickItem::widthChanged);
	auto contentHeight = QSignalSpy(surface.contentItem(), &QQuickItem::heightChanged);

	// Observe the intermediate detached state too, as QML handlers can read it.
	auto stayedVisible = true;
	auto stayedSized = true;
	QObject::connect(surface.contentItem(), &QQuickItem::windowChanged, &surface, [&]() {
		stayedVisible &= surface.isVisible();
		stayedSized &= surface.width() == 1280 && surface.height() == 720;
	});

	auto oldWindow = QPointer(window);
	surface.recreateWindow();
	QVERIFY(oldWindow.isNull());
	window = surface.contentItem()->window();
	QVERIFY(stayedVisible);
	QVERIFY(stayedSized);
	QCOMPARE(window->size(), QSize(1280, 720));
	QCOMPARE(surface.contentItem()->size(), QSizeF(1280, 720));
	QCOMPARE(window->color(), QColor(Qt::red));
	QCOMPARE(window->activeFocusItem(), &input);
	QCOMPARE(input.property("authenticationState").toString(), "pending");

	window->show();
	QCOMPARE(width.count(), 0);
	QCOMPARE(height.count(), 0);
	QCOMPARE(visible.count(), 0);
	QCOMPARE(color.count(), 0);
	QCOMPARE(contentWidth.count(), 0);
	QCOMPARE(contentHeight.count(), 0);

	// A real configure change must still update both properties and QML content.
	window->resize(1920, 1080);
	QTRY_COMPARE(surface.width(), 1920);
	QTRY_COMPARE(surface.height(), 1080);
	QCOMPARE(surface.contentItem()->size(), QSizeF(1920, 1080));
	QCOMPARE(width.count(), 1);
	QCOMPARE(height.count(), 1);
}

void TestSessionLockSurface::colorIsRetainedAfterDisown() {
	auto surface = WlSessionLockSurface();
	surface.reload(nullptr);
	auto color = QSignalSpy(&surface, &WlSessionLockSurface::colorChanged);
	surface.setColor(Qt::red);
	QCOMPARE(color.count(), 1);
	surface.setColor(Qt::red);
	QCOMPARE(color.count(), 1);
	auto window = QScopedPointer(surface.disownWindow());
	QCOMPARE(surface.color(), QColor(Qt::red));
}

void TestSessionLockSurface::lockReloadTransfersPendingRetry() {
	// This test acquires a real compositor lock. Run only in a disposable session.
	if (!qEnvironmentVariableIsSet("QS_TEST_SESSION_LOCK")
	    || QGuiApplication::platformName() != "wayland")
	{
		QSKIP("Requires an explicitly enabled disposable Wayland session");
	}

	qmlRegisterType<WlSessionLockSurface>("SessionLockTest", 1, 0, "LockSurface");
	auto engine = QQmlEngine();
	auto* component = new QQmlComponent(&engine);
	component->setData("import SessionLockTest; LockSurface {}", QUrl());
	QVERIFY2(component->isReady(), qPrintable(component->errorString()));

	// Declare the new owner first so it outlives the disowned lock on assertion failures.
	auto reloaded = WlSessionLock();
	auto old = WlSessionLock();
	auto unlock = qScopeGuard([&]() {
		if (reloaded.isLocked()) reloaded.setLocked(false);
		else old.setLocked(false);
		// Send the unlock request before the short-lived test process disconnects.
		QGuiApplication::sync();
	});
	old.setSurfaceComponent(component);
	old.setLocked(true);
	old.reload(nullptr);
	QVERIFY(old.isLocked());
	auto* oldSurface = old.findChild<WlSessionLockSurface*>();
	QVERIFY(oldSurface);
	auto* window = oldSurface->contentItem()->window();
	QTRY_VERIFY(oldSurface->width() > 0);
	QTRY_VERIFY(old.isSecure());
	oldSurface->renderRetryTimer.setInterval(8000);
	emit window->sceneGraphError(QQuickWindow::ContextNotAvailable, "test initialization failure");

	reloaded.setSurfaceComponent(component);
	reloaded.setLocked(true);
	reloaded.reload(&old);
	QVERIFY(!old.isLocked());
	auto* surface = reloaded.findChild<WlSessionLockSurface*>();
	QVERIFY(surface);
	QCOMPARE(surface->contentItem()->window(), window);
	QVERIFY(!oldSurface->renderRetryTimer.isActive());
	QVERIFY(surface->renderRetryTimer.isActive());
	QCOMPARE(surface->renderRetryTimer.interval(), 8000);
	QCOMPARE(surface->width(), window->width());
	QCOMPARE(surface->height(), window->height());
	QVERIFY(surface->isVisible());
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
