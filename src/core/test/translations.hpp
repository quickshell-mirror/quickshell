#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestTranslations: public QObject {
	Q_OBJECT;

private slots:
	static void catalogAndLanguageSwitch();
};
