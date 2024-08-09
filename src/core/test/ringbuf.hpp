#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

class TestObject {
public:
	explicit TestObject(quint32* count);
	~TestObject();
	Q_DISABLE_COPY_MOVE(TestObject);

private:
	quint32* count;
};

class TestRingBuffer: public QObject {
	Q_OBJECT;

private slots:
	static void fill();
	static void clearPartial();
	static void move();

	static void hashLookup();
};
