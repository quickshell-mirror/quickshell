#include "ringbuf.hpp"
#include <utility>

#include <qlogging.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtypes.h>

#include "../ringbuf.hpp"

TestObject::TestObject(quint32* count): count(count) {
	(*this->count)++;
	qDebug() << "Created TestObject" << this << "- count is now" << *this->count;
}

TestObject::~TestObject() {
	(*this->count)--;
	qDebug() << "Destroyed TestObject" << this << "- count is now" << *this->count;
}

void TestRingBuffer::fill() {
	quint32 counter = 0;
	auto rb = RingBuffer<TestObject>(3);
	QCOMPARE(rb.capacity(), 3);

	qInfo() << "adding test objects";
	auto* n1 = &rb.emplace(&counter);
	auto* n2 = &rb.emplace(&counter);
	auto* n3 = &rb.emplace(&counter);
	QCOMPARE(counter, 3);
	QCOMPARE(rb.size(), 3);
	QCOMPARE(&rb.at(0), n3);
	QCOMPARE(&rb.at(1), n2);
	QCOMPARE(&rb.at(2), n1);

	qInfo() << "replacing last object with new one";
	auto* n4 = &rb.emplace(&counter);
	QCOMPARE(counter, 3);
	QCOMPARE(rb.size(), 3);
	QCOMPARE(&rb.at(0), n4);
	QCOMPARE(&rb.at(1), n3);
	QCOMPARE(&rb.at(2), n2);

	qInfo() << "replacing the rest";
	auto* n5 = &rb.emplace(&counter);
	auto* n6 = &rb.emplace(&counter);
	QCOMPARE(counter, 3);
	QCOMPARE(rb.size(), 3);
	QCOMPARE(&rb.at(0), n6);
	QCOMPARE(&rb.at(1), n5);
	QCOMPARE(&rb.at(2), n4);

	qInfo() << "clearing buffer";
	rb.clear();
	QCOMPARE(counter, 0);
	QCOMPARE(rb.size(), 0);
}

void TestRingBuffer::clearPartial() {
	quint32 counter = 0;
	auto rb = RingBuffer<TestObject>(2);

	qInfo() << "adding object to buffer";
	auto* n1 = &rb.emplace(&counter);
	QCOMPARE(counter, 1);
	QCOMPARE(rb.size(), 1);
	QCOMPARE(&rb.at(0), n1);

	qInfo() << "clearing buffer";
	rb.clear();
	QCOMPARE(counter, 0);
	QCOMPARE(rb.size(), 0);
}

void TestRingBuffer::move() {
	quint32 counter = 0;

	{
		auto rb1 = RingBuffer<TestObject>(1);

		qInfo() << "adding object to first buffer";
		auto* n1 = &rb1.emplace(&counter);
		QCOMPARE(counter, 1);
		QCOMPARE(rb1.size(), 1);
		QCOMPARE(&rb1.at(0), n1);

		qInfo() << "move constructing new buffer";
		auto rb2 = RingBuffer<TestObject>(std::move(rb1));
		QCOMPARE(counter, 1);
		QCOMPARE(rb2.size(), 1);
		QCOMPARE(&rb2.at(0), n1);

		qInfo() << "move assigning new buffer";
		auto rb3 = RingBuffer<TestObject>();
		rb3 = std::move(rb2);
		QCOMPARE(counter, 1);
		QCOMPARE(rb3.size(), 1);
		QCOMPARE(&rb3.at(0), n1);
	}

	QCOMPARE(counter, 0);
}

void TestRingBuffer::hashLookup() {
	auto hb = HashBuffer<int>(3);

	qInfo() << "inserting 1,2,3 into HashBuffer";
	hb.emplace(1);
	hb.emplace(2);
	hb.emplace(3);

	qInfo() << "checking lookups";
	QCOMPARE(hb.indexOf(3), 0);
	QCOMPARE(hb.indexOf(2), 1);
	QCOMPARE(hb.indexOf(1), 2);

	qInfo() << "adding 4";
	hb.emplace(4);
	QCOMPARE(hb.indexOf(4), 0);
	QCOMPARE(hb.indexOf(3), 1);
	QCOMPARE(hb.indexOf(2), 2);
	QCOMPARE(hb.indexOf(1), -1);
}

QTEST_MAIN(TestRingBuffer);
