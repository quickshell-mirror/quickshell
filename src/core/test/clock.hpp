#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestClock: public QObject {
	Q_OBJECT;

private slots:
	static void init();
	static void resume_data(); // NOLINT(readability-identifier-naming): QtTest data provider.
	static void resume();
	static void clockSetBackwards();
	static void disabled();
	static void changePrecision();
	static void disableFromNotification();
	static void successiveDeadlines();
	static void normalTick();
};
