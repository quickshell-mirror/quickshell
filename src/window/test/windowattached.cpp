#include "windowattached.hpp"

#include <qobject.h>
#include <qquickitem.h>
#include <qscopedpointer.h>
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

void TestWindowAttachment::earlyAttachReloaded() {
	auto window1 = QScopedPointer(new ProxyWindowBase());
	auto item1 = QScopedPointer(new QQuickItem());
	item1->setParentItem(window1->contentItem());
	window1->reload(nullptr);

	auto window2 = ProxyWindowBase();
	auto item2 = QQuickItem();
	item2.setParentItem(window2.contentItem());

	auto* attached = WindowInterface::qmlAttachedProperties(&item2);
	QCOMPARE_NE(attached, nullptr);
	QCOMPARE(attached->window(), nullptr);

	auto spy = QSignalSpy(attached, &QsWindowAttached::windowChanged);
	window2.reload(window1.get());

	QCOMPARE(attached->window(), &window2);
	QCOMPARE(spy.length(), 1);

	item1.reset();
	window1.reset();

	QCOMPARE(attached->window(), &window2);
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
