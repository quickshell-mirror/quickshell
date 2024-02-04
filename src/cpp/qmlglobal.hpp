#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlscreen.hpp"

class QtShellGlobal: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<QtShellScreenInfo> screens READ screens NOTIFY screensChanged);
	QML_SINGLETON;
	QML_ELEMENT;

public:
	QtShellGlobal(QObject* parent = nullptr);

	QQmlListProperty<QtShellScreenInfo> screens();

signals:
	void screensChanged();

public slots:
	void updateScreens();

private:
	static qsizetype screensCount(QQmlListProperty<QtShellScreenInfo>* prop);
	static QtShellScreenInfo* screenAt(QQmlListProperty<QtShellScreenInfo>* prop, qsizetype i);

	QVector<QtShellScreenInfo*> mScreens;
};
