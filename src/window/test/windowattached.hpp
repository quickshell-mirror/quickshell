#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

class TestWindowAttachment: public QObject {
	Q_OBJECT;

private slots:
	static void attachedAfterReload();
	static void attachedBeforeReload();
	static void earlyAttachReloaded();
	static void owningWindowChanged();
	static void nonItemParents();
};
