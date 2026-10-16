#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestProxiedWindow: public QObject {
	Q_OBJECT;

private slots:
	void contextMenuWithoutPositionAndFocusItem();
};
