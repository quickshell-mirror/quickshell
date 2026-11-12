#include "scriptmodel.hpp"

#include <qabstractitemmodel.h>
#include <qabstractitemmodeltester.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qhash.h>
#include <qjsengine.h>
#include <qjsvalue.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qstring.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtypes.h>

#include "../scriptmodel.hpp"

bool ModelOperation::operator==(const ModelOperation& other) const {
	return other.operation == this->operation && other.index == this->index
	    && other.length == this->length && other.destIndex == this->destIndex;
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
QDebug& operator<<(QDebug& debug, const ModelOperation& op) {
	auto saver = QDebugStateSaver(debug);
	debug.nospace();

	switch (op.operation) {
	case ModelOperation::Insert: debug << "Insert"; break;
	case ModelOperation::Remove: debug << "Remove"; break;
	case ModelOperation::Move: debug << "Move"; break;
	}

	debug << "(i: " << op.index << ", l: " << op.length;

	if (op.destIndex != -1) {
		debug << ", d: " << op.destIndex;
	}

	debug << ')';

	return debug;
}

ChangeObserver::ChangeObserver(QAbstractItemModel* model) {
	QObject::connect(model, &QAbstractItemModel::rowsInserted, this, &ChangeObserver::onInsert);
	QObject::connect(model, &QAbstractItemModel::rowsRemoved, this, &ChangeObserver::onRemove);
	QObject::connect(model, &QAbstractItemModel::rowsMoved, this, &ChangeObserver::onMove);
	QObject::connect(model, &QAbstractItemModel::dataChanged, this, &ChangeObserver::onDataChanged);
}

void ChangeObserver::onInsert(const QModelIndex& parent, int first, int last) {
	QCOMPARE(parent, QModelIndex());
	this->mOperations.emplaceBack(ModelOperation::Insert, first, last - first + 1);
}

void ChangeObserver::onRemove(const QModelIndex& parent, int first, int last) {
	QCOMPARE(parent, QModelIndex());
	this->mOperations.emplaceBack(ModelOperation::Remove, first, last - first + 1);
}

void ChangeObserver::onMove(
    const QModelIndex& sourceParent,
    int sourceStart,
    int sourceEnd,
    const QModelIndex& destParent,
    int destStart
) {
	QCOMPARE(sourceParent, QModelIndex());
	QCOMPARE(destParent, QModelIndex());
	this->mOperations
	    .emplaceBack(ModelOperation::Move, sourceStart, sourceEnd - sourceStart + 1, destStart);
}

void ChangeObserver::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
	this->mChanges.emplaceBack(topLeft.row(), bottomRight.row());
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
QDebug& operator<<(QDebug& debug, const QJSValueList& list) {
	QString str;

	for (const auto& var: list) {
		if (var.isString()) {
			str += var.toString();
		} else {
			qFatal() << "QJSValueList debug overridden in test";
		}
	}

	debug << str;
	return debug;
}

namespace {
bool qjsValueListsEqual(const QJSValueList& a, const QJSValueList& b) {
	if (a.length() != b.length()) return false;

	for (auto i = 0; i != a.length(); i++) {
		if (!a.at(i).strictlyEquals(b.at(i))) return false;
	}

	return true;
}
} // namespace

void TestScriptModel::unique_data() {
	QTest::addColumn<QString>("oldstr");
	QTest::addColumn<QString>("newstr");
	QTest::addColumn<OpList>("operations");

	QTest::addRow("append") << "ABCD" << "ABCDEFG" << OpList({{ModelOperation::Insert, 4, 3}});

	QTest::addRow("prepend") << "EFG" << "ABCDEFG" << OpList({{ModelOperation::Insert, 0, 4}});

	QTest::addRow("insert") << "ABFG" << "ABCDEFG" << OpList({{ModelOperation::Insert, 2, 3}});

	QTest::addRow("chop") << "ABCDEFG" << "ABCD" << OpList({{ModelOperation::Remove, 4, 3}});

	QTest::addRow("slice") << "ABCDEFG" << "DEFG" << OpList({{ModelOperation::Remove, 0, 3}});

	QTest::addRow("remove_mid") << "ABCDEFG" << "ABFG" << OpList({{ModelOperation::Remove, 2, 3}});

	QTest::addRow("move_single") << "ABCDEFG" << "AFBCDEG"
	                             << OpList({{ModelOperation::Move, 5, 1, 1}});

	QTest::addRow("move_range") << "ABCDEFG" << "ADEFBCG"
	                            << OpList({{ModelOperation::Move, 3, 3, 1}});

	// beginning to end is the same operation
	QTest::addRow("move_end_to_beginning")
	    << "ABCDEFG" << "EFGABCD" << OpList({{ModelOperation::Move, 4, 3, 0}});

	QTest::addRow("move_overlapping")
	    << "ABCDEFG" << "ABDEFCG" << OpList({{ModelOperation::Move, 3, 3, 2}});

	// Ensure iterators aren't skipping anything at the end of operations by performing
	// multiple back to back.

	QTest::addRow("insert_state_ok") << "ABCDEFG" << "ABXXEFG"
	                                 << OpList({
	                                        {ModelOperation::Insert, 2, 2}, // ABXXCDEFG
	                                        {ModelOperation::Remove, 4, 2}, // ABXXEFG
	                                    });

	QTest::addRow("remove_state_ok") << "ABCDEFG" << "ABFGE"
	                                 << OpList({
	                                        {ModelOperation::Remove, 2, 2},  // ABEFG
	                                        {ModelOperation::Move, 3, 2, 2}, // ABFGE
	                                    });

	QTest::addRow("move_state_ok") << "ABCDEFG" << "ABEFXYCDG"
	                               << OpList({
	                                      {ModelOperation::Move, 4, 2, 2}, // ABEFCDG
	                                      {ModelOperation::Insert, 4, 2},  // ABEFXYCDG
	                                  });
}

void TestScriptModel::unique() {
	QFETCH(const QString, oldstr);
	QFETCH(const QString, newstr);
	QFETCH(const OpList, operations);

	auto strToJsValueList = [](const QString& str) -> QJSValueList {
		QJSValueList list;

		for (auto c: str) {
			list.emplace_back(QString(c));
		}

		return list;
	};

	auto oldList = strToJsValueList(oldstr);
	auto newList = strToJsValueList(newstr);

	ScriptModel model;
	auto modelTester = QAbstractItemModelTester(&model);
	auto observer = ChangeObserver(&model);

	model.setValues(oldList);
	QVERIFY(qjsValueListsEqual(model.values(), oldList));
	QCOMPARE_EQ(
	    observer.operations(),
	    OpList({{ModelOperation::Insert, 0, static_cast<qint32>(oldList.length())}})
	);

	observer.clear();

	model.setValues(newList);
	QVERIFY(qjsValueListsEqual(model.values(), newList));
	QCOMPARE_EQ(observer.operations(), operations);
	QCOMPARE_EQ(observer.changes(), ChangeList());
}

void TestScriptModel::structuralEquality() {
	ScriptModel model;
	QJSEngine engine;

	auto object = engine.newObject();
	object.setProperty("foo", "bar");
	object.setProperty("bar", "baz");
	object.setProperty("subobject", engine.newObject());

	auto replacement = engine.newObject();
	replacement.setProperty("subobject", engine.newObject());
	replacement.setProperty("bar", "baz");
	replacement.setProperty("foo", "bar");

	auto oldList = QList<QJSValue> {object};

	auto newList = QList<QJSValue> {replacement};

	auto modelTester = QAbstractItemModelTester(&model);
	auto observer = ChangeObserver(&model);

	model.setValues(oldList);
	QVERIFY(qjsValueListsEqual(model.values(), oldList));
	QCOMPARE_EQ(
	    observer.operations(),
	    OpList({{ModelOperation::Insert, 0, static_cast<qint32>(oldList.length())}})
	);

	observer.clear();

	model.setValues(newList);
	QVERIFY(qjsValueListsEqual(model.values(), newList));
	QCOMPARE_EQ(observer.operations(), OpList());

	auto structurallyEquals = [](const QJSValue& a, const QJSValue& b) {
		ScriptModel model;
		model.setValues({a});

		auto observer = ChangeObserver(&model);
		model.setValues({b});
		return observer.operations().isEmpty();
	};

	auto different = engine.newObject();
	different.setProperty("foo", "bar");
	different.setProperty("bar", "baz");
	auto differentSubobject = engine.newObject();
	differentSubobject.setProperty("value", 1);
	different.setProperty("subobject", differentSubobject);
	QVERIFY(!structurallyEquals(object, different));

	auto cycleA = engine.newObject();
	cycleA.setProperty("self", cycleA);
	auto cycleB = engine.newObject();
	cycleB.setProperty("self", cycleB);
	QVERIFY(!structurallyEquals(cycleA, cycleB));

	const auto arrayA = engine.evaluate("[1, { value: 'x' }]");
	const auto arrayB = engine.evaluate("[1, { value: 'x' }]");
	QVERIFY(structurallyEquals(arrayA, arrayB));

	auto sparseArrayA = engine.newArray(1);
	auto sparseArrayB = engine.newArray(2);
	QVERIFY(!structurallyEquals(sparseArrayA, sparseArrayB));
}

void TestScriptModel::comparisonModes() {
	QJSEngine engine;
	auto makeValue = [&engine]() {
		auto value = engine.newObject();
		value.setProperty("nested", engine.newObject());
		return value;
	};

	ScriptModel model;
	QCOMPARE_EQ(model.comparisonMode(), ObjectComparison::Structure);

	model.setValues({makeValue()});
	model.setComparisonMode(ObjectComparison::Identity);
	QCOMPARE_EQ(model.comparisonMode(), ObjectComparison::Identity);

	auto observer = ChangeObserver(&model);
	model.setValues({makeValue()});
	QVERIFY(!observer.operations().isEmpty());

	model.setComparisonMode(ObjectComparison::Structure);

	observer.clear();
	model.setValues({makeValue()});
	QCOMPARE_EQ(observer.operations(), OpList());
}

void TestScriptModel::dataUpdated_data() {
	QTest::addColumn<QString>("oldstr");
	QTest::addColumn<QString>("newstr");
	QTest::addColumn<OpList>("operations");
	QTest::addColumn<ChangeList>("changes");

	// Uppercase = same data, Lowercase = data updated
	QTest::addRow("insert") << "ABC" << "AXBC" << OpList({{ModelOperation::Insert, 1, 1}})
	                        << ChangeList();
	QTest::addRow("remove") << "AXBC" << "ABC" << OpList({{ModelOperation::Remove, 1, 1}})
	                        << ChangeList();
	QTest::addRow("move") << "ABCD" << "ACBD" << OpList({{ModelOperation::Move, 2, 1, 1}})
	                      << ChangeList();
	QTest::addRow("move_range") << "ABCDEF" << "ADEBCF" << OpList({{ModelOperation::Move, 3, 2, 1}})
	                            << ChangeList();
	QTest::addRow("state_ok") << "ABCDEFG" << "ABEFXYCDG"
	                          << OpList({
	                                 {ModelOperation::Move, 4, 2, 2}, // ABEFCDG
	                                 {ModelOperation::Insert, 4, 2},  // ABEFXYCDG
	                             })
	                          << ChangeList();

	QTest::addRow("insert_all_updated") << "ABC" << "aXbc" << OpList({{ModelOperation::Insert, 1, 1}})
	                                    << ChangeList({{0, 0}, {2, 3}});
	QTest::addRow("remove_all_updated")
	    << "AXBC" << "abc" << OpList({{ModelOperation::Remove, 1, 1}}) << ChangeList({{0, 2}});
	QTest::addRow("move_all_updated")
	    << "ABCD" << "acbd" << OpList({{ModelOperation::Move, 2, 1, 1}}) << ChangeList({{0, 3}});
	QTest::addRow("move_range_all_updated")
	    << "ABCDEF" << "adebcf" << OpList({{ModelOperation::Move, 3, 2, 1}}) << ChangeList({{0, 5}});
	QTest::addRow("state_ok_all_updated") << "ABCDEFG" << "abefXYcdg"
	                                      << OpList({
	                                             {ModelOperation::Move, 4, 2, 2}, // abefCDG
	                                             {ModelOperation::Insert, 4, 2},  // abefXYCDG
	                                         })
	                                      << ChangeList({{0, 3}, {6, 8}});

	QTest::addRow("update_in_place") << "ABC" << "aBc" << OpList() << ChangeList({{0, 0}, {2, 2}});
	QTest::addRow("move_updated") << "ABCDEF" << "AdeBCF" << OpList({{ModelOperation::Move, 3, 2, 1}})
	                              << ChangeList({{1, 2}});
	QTest::addRow("move_neighbours_updated")
	    << "ABCD" << "aCbd" << OpList({{ModelOperation::Move, 2, 1, 1}})
	    << ChangeList({{0, 0}, {2, 3}});
	QTest::addRow("move_partially_updated")
	    << "ABCDEF" << "aCdEbf" << OpList({{ModelOperation::Move, 2, 3, 1}})
	    << ChangeList({{0, 0}, {2, 2}, {4, 5}});
}

void TestScriptModel::dataUpdated() {
	QFETCH(const QString, oldstr);
	QFETCH(const QString, newstr);
	QFETCH(const OpList, operations);
	QFETCH(const ChangeList, changes);

	QJSEngine engine;
	auto version = 0;
	QHash<QString, QJSValue> oldValues;

	auto strToJsValueList = [&](const QString& str) -> QJSValueList {
		QJSValueList list;

		for (auto c: str) {
			auto key = QString(c.toUpper());

			if (c.isUpper() && oldValues.contains(key)) {
				list.emplace_back(oldValues.value(key));
				continue;
			}

			auto value = engine.newObject();
			value.setProperty("key", key);
			value.setProperty("version", version);
			list.emplace_back(value);
		}

		version++;
		return list;
	};

	auto oldList = strToJsValueList(oldstr);
	for (const auto& value: oldList) oldValues.insert(value.property("key").toString(), value);
	auto newList = strToJsValueList(newstr);

	ScriptModel model;
	model.setObjectProp("key");
	auto modelTester = QAbstractItemModelTester(&model);

	model.setValues(oldList);
	auto observer = ChangeObserver(&model);

	model.setValues(newList);
	QVERIFY(qjsValueListsEqual(model.values(), newList));
	QCOMPARE_EQ(observer.operations(), operations);
	QCOMPARE_EQ(observer.changes(), changes);
}

QTEST_MAIN(TestScriptModel);
