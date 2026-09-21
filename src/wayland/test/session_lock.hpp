#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestSessionLock: public QObject {
	Q_OBJECT;

private slots:
	static void initialGraphicsError();
	static void initializedSurfaceDoesNotAbort();
	static void disownedWindowDoesNotAbort();
	static void failedLockUnlocks();
	static void staleFailureDoesNotUnlock();
	static void transferredManagerIgnoresFailure_data(); // NOLINT(readability-identifier-naming)
	static void transferredManagerIgnoresFailure();
	static void normalUnlockNotifies();
};
