#include "transformwatcher.hpp"

#include <qlist.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../transformwatcher.hpp"

void TestTransformWatcher::aParentOfB() { // NOLINT
	auto a = QQuickItem();
	a.setObjectName("a");
	auto b = QQuickItem();
	b.setObjectName("b");
	b.setParentItem(&a);

	auto watcher = TransformWatcher();
	watcher.setA(&a);
	watcher.setB(&b);

	QCOMPARE(watcher.parentChain, {&a});
	QCOMPARE(watcher.childChain, {&b});
}

void TestTransformWatcher::bParentOfA() { // NOLINT
	auto a = QQuickItem();
	a.setObjectName("a");
	auto b = QQuickItem();
	b.setObjectName("b");
	a.setParentItem(&b);

	auto watcher = TransformWatcher();
	watcher.setA(&a);
	watcher.setB(&b);

	QCOMPARE(watcher.parentChain, (QList {&a, &b}));
	QCOMPARE(watcher.childChain, {});
}

// a
//  p1    b
//   p2 c1
//    p3
void TestTransformWatcher::aParentChainB() { // NOLINT
	auto a = QQuickItem();
	a.setObjectName("a");
	auto b = QQuickItem();
	b.setObjectName("b");

	auto p1 = QQuickItem();
	p1.setObjectName("p1");
	auto p2 = QQuickItem();
	p2.setObjectName("p2");
	auto p3 = QQuickItem();
	p3.setObjectName("p3");
	auto c1 = QQuickItem();
	c1.setObjectName("c1");

	a.setParentItem(&p1);
	p1.setParentItem(&p2);
	p2.setParentItem(&p3);

	b.setParentItem(&c1);
	c1.setParentItem(&p3);

	auto watcher = TransformWatcher();
	watcher.setA(&a);
	watcher.setB(&b);

	QCOMPARE(watcher.parentChain, (QList {&a, &p1, &p2, &p3}));
	QCOMPARE(watcher.childChain, (QList {&b, &c1}));
}

void TestTransformWatcher::multiWindow() { // NOLINT
	auto a = QQuickItem();
	a.setObjectName("a");
	auto b = QQuickItem();
	b.setObjectName("b");

	auto p = QQuickItem();
	p.setObjectName("p");
	auto c = QQuickItem();
	c.setObjectName("c");

	a.setParentItem(&p);
	b.setParentItem(&c);

	auto aW = QQuickWindow();
	auto bW = QQuickWindow();

	p.setParentItem(aW.contentItem());
	c.setParentItem(bW.contentItem());

	auto watcher = TransformWatcher();
	watcher.setA(&a);
	watcher.setB(&b);

	QCOMPARE(watcher.parentChain, (QList {&a, &p, aW.contentItem()}));
	QCOMPARE(watcher.childChain, (QList {&b, &c, bW.contentItem()}));
	QCOMPARE(watcher.parentWindow, &aW);
	QCOMPARE(watcher.childWindow, &bW);
}

QTEST_MAIN(TestTransformWatcher);
