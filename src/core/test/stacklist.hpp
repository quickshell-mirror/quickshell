#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestStackList: public QObject {
	Q_OBJECT;

private slots:
	static void push();
	static void pushAndGrow();
	static void copy();
	static void viewVla();
	static void viewVlaGrown();
};
