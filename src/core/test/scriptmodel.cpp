#include "scriptmodel.hpp"

#include <qabstractitemmodel.h>
#include <qabstractitemmodeltester.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qsignalspy.h>
#include <qstring.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtypes.h>

#include "../scriptmodel.hpp"

using OpList = QList<ModelOperation>;

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

// NOLINTNEXTLINE(misc-use-internal-linkage)
QDebug& operator<<(QDebug& debug, const QVariantList& list) {
	auto str = QString();

	for (const auto& var: list) {
		if (var.canConvert<QChar>()) {
			str += var.value<QChar>();
		} else {
			qFatal() << "QVariantList debug overridden in test";
		}
	}

	debug << str;
	return debug;
}

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

	// Ensure iterators arent skipping anything at the end of operations by performing
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

	auto strToVariantList = [](const QString& str) -> QVariantList {
		QVariantList list;

		for (auto c: str) {
			list.emplace_back(c);
		}

		return list;
	};

	auto oldlist = strToVariantList(oldstr);
	auto newlist = strToVariantList(newstr);

	auto model = ScriptModel();
	auto modelTester = QAbstractItemModelTester(&model);

	OpList actualOperations;

	auto onInsert = [&](const QModelIndex& parent, int first, int last) {
		QCOMPARE(parent, QModelIndex());
		actualOperations << ModelOperation(ModelOperation::Insert, first, last - first + 1);
	};

	auto onRemove = [&](const QModelIndex& parent, int first, int last) {
		QCOMPARE(parent, QModelIndex());
		actualOperations << ModelOperation(ModelOperation::Remove, first, last - first + 1);
	};

	auto onMove = [&](const QModelIndex& sourceParent,
	                  int sourceStart,
	                  int sourceEnd,
	                  const QModelIndex& destParent,
	                  int destStart) {
		QCOMPARE(sourceParent, QModelIndex());
		QCOMPARE(destParent, QModelIndex());
		actualOperations << ModelOperation(
		    ModelOperation::Move,
		    sourceStart,
		    sourceEnd - sourceStart + 1,
		    destStart
		);
	};

	QObject::connect(&model, &QAbstractItemModel::rowsInserted, &model, onInsert);
	QObject::connect(&model, &QAbstractItemModel::rowsRemoved, &model, onRemove);
	QObject::connect(&model, &QAbstractItemModel::rowsMoved, &model, onMove);

	model.setValues(oldlist);
	QCOMPARE_EQ(model.values(), oldlist);
	QCOMPARE_EQ(
	    actualOperations,
	    OpList({{ModelOperation::Insert, 0, static_cast<qint32>(oldlist.length())}})
	);

	actualOperations.clear();

	model.setValues(newlist);
	QCOMPARE_EQ(model.values(), newlist);
	QCOMPARE_EQ(actualOperations, operations);
}

QTEST_MAIN(TestScriptModel);
