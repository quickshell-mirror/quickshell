#include "objectmodel.hpp"

#include <qlist.h>
#include <qobject.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../model.hpp"

void TestObjectModel::diffUpdateInsertRemove() {
	QObject a;
	QObject b;
	QObject c;
	QObject d;

	auto model = ObjectModel<QObject>(nullptr);
	model.insertObject(&a);
	model.insertObject(&b);
	model.insertObject(&c);
	QCOMPARE(model.valueList(), (QList<QObject*> {&a, &b, &c}));

	// drop b, append d — relative order of survivors unchanged
	model.diffUpdate({&a, &c, &d});
	QCOMPARE(model.valueList(), (QList<QObject*> {&a, &c, &d}));
}

void TestObjectModel::diffUpdateReorder() {
	QObject a;
	QObject b;
	QObject c;
	QObject d;

	auto model = ObjectModel<QObject>(nullptr);
	model.insertObject(&d);
	model.insertObject(&a);
	model.insertObject(&b);
	model.insertObject(&c);
	QCOMPARE(model.valueList(), (QList<QObject*> {&d, &a, &b, &c}));

	// pure permutation: same elements, different order. Previously turned
	// [d,a,b,c] into [a,b,c,d,a,b,c] because the old row was never removed
	// before inserting at the new index.
	model.diffUpdate({&a, &b, &c, &d});
	QCOMPARE(model.valueList(), (QList<QObject*> {&a, &b, &c, &d}));
}

QTEST_MAIN(TestObjectModel);
