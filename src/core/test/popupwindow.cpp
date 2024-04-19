#include "popupwindow.hpp"

#include <qquickwindow.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qwindow.h>

#include "../popupwindow.hpp"
#include "../proxywindow.hpp"

void TestPopupWindow::initiallyVisible() { // NOLINT
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	popup.setParentWindow(&parent);
	popup.setVisible(true);

	parent.reload();
	popup.reload();

	QVERIFY(popup.isVisible());
	QVERIFY(popup.backingWindow()->isVisible());
	QCOMPARE(popup.backingWindow()->transientParent(), parent.backingWindow());
}

void TestPopupWindow::reloadReparent() { // NOLINT
	// first generation
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	auto* win2 = new QQuickWindow();
	win2->setVisible(true);

	parent.setVisible(true);
	popup.setParentWindow(&parent);
	popup.setVisible(true);

	parent.reload();
	popup.reload();

	// second generation
	auto newParent = ProxyWindowBase();
	auto newPopup = ProxyPopupWindow();

	newPopup.setParentWindow(&newParent);
	newPopup.setVisible(true);

	auto* oldWindow = popup.backingWindow();
	auto* oldTransientParent = oldWindow->transientParent();

	auto spy = QSignalSpy(oldWindow, &QWindow::visibleChanged);

	newParent.reload(&parent);
	newPopup.reload(&popup);

	QVERIFY(newPopup.isVisible());
	QVERIFY(newPopup.backingWindow()->isVisible());
	QCOMPARE(newPopup.backingWindow()->transientParent(), oldTransientParent);
	QCOMPARE(newPopup.backingWindow()->transientParent(), newParent.backingWindow());
	QCOMPARE(spy.length(), 0);
}

void TestPopupWindow::reloadUnparent() { // NOLINT
	// first generation
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	popup.setParentWindow(&parent);
	popup.setVisible(true);

	parent.reload();
	popup.reload();

	// second generation
	auto newPopup = ProxyPopupWindow();

	// parent not set
	newPopup.setVisible(true);
	newPopup.reload(&popup);

	QVERIFY(!newPopup.isVisible());
	QVERIFY(!newPopup.backingWindow()->isVisible());
	QCOMPARE(newPopup.backingWindow()->transientParent(), nullptr);
}

void TestPopupWindow::invisibleWithoutParent() { // NOLINT
	auto popup = ProxyPopupWindow();

	popup.setVisible(true);
	popup.reload();

	QVERIFY(!popup.isVisible());
}

void TestPopupWindow::moveWithParent() { // NOLINT
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	popup.setParentWindow(&parent);
	popup.setRelativeX(10);
	popup.setRelativeY(10);
	popup.setVisible(true);

	parent.reload();
	popup.reload();

	QCOMPARE(popup.x(), parent.x() + 10);
	QCOMPARE(popup.y(), parent.y() + 10);

	parent.backingWindow()->setX(10);
	parent.backingWindow()->setY(10);

	QCOMPARE(popup.x(), parent.x() + 10);
	QCOMPARE(popup.y(), parent.y() + 10);
}

void TestPopupWindow::attachParentLate() { // NOLINT
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	popup.setVisible(true);

	parent.reload();
	popup.reload();

	QVERIFY(!popup.isVisible());

	popup.setParentWindow(&parent);
	QVERIFY(popup.isVisible());
	QVERIFY(popup.backingWindow()->isVisible());
	QCOMPARE(popup.backingWindow()->transientParent(), parent.backingWindow());
}

void TestPopupWindow::reparentLate() { // NOLINT
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	popup.setParentWindow(&parent);
	popup.setVisible(true);

	parent.reload();
	popup.reload();

	QCOMPARE(popup.x(), parent.x());
	QCOMPARE(popup.y(), parent.y());

	auto parent2 = ProxyWindowBase();
	parent2.reload();

	parent2.backingWindow()->setX(10);
	parent2.backingWindow()->setY(10);

	popup.setParentWindow(&parent2);
	QVERIFY(popup.isVisible());
	QVERIFY(popup.backingWindow()->isVisible());
	QCOMPARE(popup.backingWindow()->transientParent(), parent2.backingWindow());
	QCOMPARE(popup.x(), parent2.x());
	QCOMPARE(popup.y(), parent2.y());
}

void TestPopupWindow::xMigrationFix() { // NOLINT
	auto parent = ProxyWindowBase();
	auto popup = ProxyPopupWindow();

	popup.setParentWindow(&parent);
	popup.setVisible(true);

	parent.reload();
	popup.reload();

	QCOMPARE(popup.x(), parent.x());

	popup.setVisible(false);
	popup.setVisible(true);

	QCOMPARE(popup.x(), parent.x());
}

QTEST_MAIN(TestPopupWindow);
