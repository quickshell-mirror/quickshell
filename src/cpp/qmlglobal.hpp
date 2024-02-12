#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlscreen.hpp"

class QuickShellGlobal: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<QuickShellScreenInfo> screens READ screens NOTIFY screensChanged);
	QML_SINGLETON;
	QML_NAMED_ELEMENT(QuickShell);

public:
	QuickShellGlobal(QObject* parent = nullptr);

	QQmlListProperty<QuickShellScreenInfo> screens();

signals:
	void screensChanged();

public slots:
	void reload(bool hard);
	void updateScreens();

private:
	static qsizetype screensCount(QQmlListProperty<QuickShellScreenInfo>* prop);
	static QuickShellScreenInfo* screenAt(QQmlListProperty<QuickShellScreenInfo>* prop, qsizetype i);

	QVector<QuickShellScreenInfo*> mScreens;
};
