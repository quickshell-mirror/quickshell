#include "datastream.hpp"

#include <qbytearray.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../datastream.hpp"

void TestSplitParser::splits_data() { // NOLINT
	QTest::addColumn<QString>("mark");
	QTest::addColumn<QString>("buffer");   // max that can go in the buffer
	QTest::addColumn<QString>("incoming"); // data that has to be tested on the end in one go
	QTest::addColumn<QList<QString>>("results");
	QTest::addColumn<QString>("remainder");

	// NOLINTBEGIN
	// clang-format off
	QTest::addRow("simple") << "-"
		<< "foo" << "-"
		<< QList<QString>("foo") << "";

	QTest::addRow("multiple") << "-"
		<< "foo" << "-bar-baz-"
		<< QList<QString>({ "foo", "bar", "baz" }) << "";

	QTest::addRow("incomplete") << "-"
		<< "foo" << "-bar-baz"
		<< QList<QString>({ "foo", "bar" }) << "baz";

	QTest::addRow("longsplit") << "12345"
		<< "foo1234" << "5bar12345"
		<< QList<QString>({ "foo", "bar" }) << "";

	QTest::addRow("longsplit-incomplete") << "123"
		<< "foo12" << "3bar123baz"
		<< QList<QString>({ "foo", "bar" }) << "baz";
	// clang-format on
	// NOLINTEND
}

void TestSplitParser::splits() { // NOLINT
	// NOLINTBEGIN
	QFETCH(QString, mark);
	QFETCH(QString, buffer);
	QFETCH(QString, incoming);
	QFETCH(QList<QString>, results);
	QFETCH(QString, remainder);
	// NOLINTEND

	auto bufferArray = buffer.toUtf8();
	auto incomingArray = incoming.toUtf8();

	for (auto i = 0; i <= bufferArray.length(); i++) {
		auto buffer = bufferArray.sliced(0, i);
		auto incoming = bufferArray.sliced(i);
		incoming.append(incomingArray);

		qInfo() << "BUFFER" << QString(buffer);
		qInfo() << "INCOMING" << QString(incoming);

		auto parser = SplitParser();
		auto spy = QSignalSpy(&parser, &DataStreamParser::read);

		parser.setSplitMarker(mark);
		parser.parseBytes(incoming, buffer);

		auto actualResults = QList<QString>();
		for (auto& read: spy) {
			actualResults.push_back(read[0].toString());
		}

		qInfo() << "EXPECTED RESULTS" << results;
		qInfo() << "ACTUAL RESULTS" << actualResults;
		qInfo() << "EXPECTED REMAINDER" << remainder;
		qInfo() << "ACTUAL REMAINDER" << remainder;
		QCOMPARE(actualResults, results);
		QCOMPARE(buffer, remainder);
	}
}

void TestSplitParser::initBuffer() { // NOLINT
	auto parser = SplitParser();
	auto spy = QSignalSpy(&parser, &DataStreamParser::read);

	auto buf = QString("foo-bar-baz").toUtf8();
	auto expected = QList<QString>({"foo", "bar"});

	parser.setSplitMarker("-");
	parser.parseBytes(buf, buf);

	auto actualResults = QList<QString>();
	for (auto& read: spy) {
		actualResults.push_back(read[0].toString());
	}

	qInfo() << "EXPECTED RESULTS" << expected;
	qInfo() << "ACTUAL RESULTS" << actualResults;
	QCOMPARE(actualResults, expected);
	QCOMPARE(buf, "baz");
}

QTEST_MAIN(TestSplitParser);
