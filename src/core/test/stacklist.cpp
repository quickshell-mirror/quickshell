#include "stacklist.hpp"
#include <cstddef>

#include <qlist.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../stacklist.hpp"

void TestStackList::push() {
	StackList<int, 2> list;

	list.push(1);
	list.push(2);

	QCOMPARE_EQ(list.toList(), QList({1, 2}));
	QCOMPARE_EQ(list.length(), 2);
}

void TestStackList::pushAndGrow() {
	StackList<int, 2> list;

	list.push(1);
	list.push(2);
	list.push(3);
	list.push(4);

	QCOMPARE_EQ(list.toList(), QList({1, 2, 3, 4}));
	QCOMPARE_EQ(list.length(), 4);
}

void TestStackList::copy() {
	StackList<int, 2> list;

	list.push(1);
	list.push(2);
	list.push(3);
	list.push(4);

	QCOMPARE_EQ(list.toList(), QList({1, 2, 3, 4}));
	QCOMPARE_EQ(list.length(), 4);

	auto list2 = list;

	QCOMPARE_EQ(list2.toList(), QList({1, 2, 3, 4}));
	QCOMPARE_EQ(list2.length(), 4);
	QCOMPARE_EQ(list2, list);
}

void TestStackList::viewVla() {
	StackList<int, 2> list;

	list.push(1);
	list.push(2);

	QCOMPARE_EQ(list.toList(), QList({1, 2}));
	QCOMPARE_EQ(list.length(), 2);

	STACKLIST_VLA_VIEW(int, list, listView);

	QList<int> ql;

	for (size_t i = 0; i != list.length(); ++i) {
		ql.push_back(listView[i]); // NOLINT
	}

	QCOMPARE_EQ(ql, list.toList());
}

void TestStackList::viewVlaGrown() {
	StackList<int, 2> list;

	list.push(1);
	list.push(2);
	list.push(3);
	list.push(4);

	QCOMPARE_EQ(list.toList(), QList({1, 2, 3, 4}));
	QCOMPARE_EQ(list.length(), 4);

	STACKLIST_VLA_VIEW(int, list, listView);

	QList<int> ql;

	for (size_t i = 0; i != list.length(); ++i) {
		ql.push_back(listView[i]); // NOLINT
	}

	QCOMPARE_EQ(ql, list.toList());
}

QTEST_MAIN(TestStackList);
