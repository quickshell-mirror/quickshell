#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestSessionLockSurface: public QObject {
	Q_OBJECT;

private slots:
	static void errorSchedulesRetry();
	static void reloadTransfersPendingRetry();
	static void frameResetsRetry();
	static void frameConnectionIsSingleShot();
	static void recreationPreservesPropertiesAndFocus();
	static void colorIsRetainedAfterDisown();
	static void lockReloadTransfersPendingRetry();
	static void unlockedSurfaceDoesNotRetry();
};
