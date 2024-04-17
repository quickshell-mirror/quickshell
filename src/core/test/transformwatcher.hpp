#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestTransformWatcher: public QObject {
	Q_OBJECT;

private slots:
	void aParentOfB();
	void bParentOfA();
	void aParentChainB();
	void multiWindow();
};
