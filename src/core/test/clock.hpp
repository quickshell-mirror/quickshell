#pragma once

#include <qbytearray.h>
#include <qobject.h>
#include <qtmetamacros.h>

class TestClock: public QObject {
	Q_OBJECT;

private slots:
	void init();
	void cleanup();
	static void timeZones_data(); // NOLINT(readability-identifier-naming): QtTest data provider.
	static void timeZones();
	static void earlyTickAcrossFallBack();

private:
	QByteArray oldTimezone;
	bool hadTimezone = false;
};
