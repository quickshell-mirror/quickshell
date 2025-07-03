#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestProcess: public QObject {
	Q_OBJECT;

private slots:
	static void startAfterReload();
	static void testExec();
};
