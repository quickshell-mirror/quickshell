#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestPopupWindow: public QObject {
	Q_OBJECT;

private slots:
	void initiallyVisible();
	void reloadReparent();
	void reloadUnparent();
	void invisibleWithoutParent();
	void moveWithParent();
	void attachParentLate();
	void reparentLate();
	void xMigrationFix();
};
