#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestSplitParser: public QObject {
	Q_OBJECT;

private slots:
	void splits_data(); // NOLINT
	void splits();
	void initBuffer();
};
