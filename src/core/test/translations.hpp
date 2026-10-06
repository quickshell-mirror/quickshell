#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestTranslations: public QObject {
	Q_OBJECT;

private slots:
	// Qt Test requires this suffix for the data provider.
	static void catalogAndLanguageSwitch_data(); // NOLINT(readability-identifier-naming)
	static void catalogAndLanguageSwitch();
};
