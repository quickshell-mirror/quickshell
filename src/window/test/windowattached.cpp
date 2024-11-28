#include "windowattached.hpp"

#include <qobject.h>
#include <qquickitem.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../proxywindow.hpp"
#include "../windowinterface.hpp"

void TestWindowAttachment::attachedAfterReload() {
	auto window = ProxyWindowBase();
	auto item = QQuickItem();
	item.setParentItem(window.contentItem());
	window.reload(nullptr);

	auto* attached = WindowInterface::qmlAttachedProperties(&item);
	QCOMPARE_NE(attached, nullptr);
	QCOMPARE(attached->window(), &window);
}

void TestWindowAttachment::attachedBeforeReload() {
	auto window = ProxyWindowBase();
	auto item = QQuickItem();
	item.setParentItem(window.contentItem());

	auto* attached = WindowInterface::qmlAttachedProperties(&item);
	QCOMPARE_NE(attached, nullptr);
	QCOMPARE(attached->window(), nullptr);

	auto spy = QSignalSpy(attached, &QsWindowAttached::windowChanged);
	window.reload(nullptr);

	QCOMPARE(attached->window(), &window);
	QCOMPARE(spy.length(), 1);
}

void TestWindowAttachment::owningWindowChanged() {
	auto window1 = ProxyWindowBase();
	auto window2 = ProxyWindowBase();
	window1.reload(nullptr);
	window2.reload(nullptr);

	auto item = QQuickItem();
	item.setParentItem(window1.contentItem());

	auto* attached = WindowInterface::qmlAttachedProperties(&item);
	QCOMPARE_NE(attached, nullptr);
	QCOMPARE(attached->window(), &window1);

	auto spy = QSignalSpy(attached, &QsWindowAttached::windowChanged);
	item.setParentItem(window2.contentItem());
	QCOMPARE(attached->window(), &window2);
	// setParentItem changes the parent to nullptr before the new window.
	QCOMPARE(spy.length(), 2);
}

void TestWindowAttachment::nonItemParents() {
	auto window = ProxyWindowBase();

	auto item = QQuickItem();
	item.setParentItem(window.contentItem());
	auto object = QObject(&item);

	window.reload(nullptr);

	auto* attached = WindowInterface::qmlAttachedProperties(&object);
	QCOMPARE_NE(attached, nullptr);
	QCOMPARE(attached->window(), &window);
}

QTEST_MAIN(TestWindowAttachment);
