#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestObjectModel: public QObject {
	Q_OBJECT;

private slots:
	static void diffUpdateInsertRemove();
	static void diffUpdateReorder();
};
