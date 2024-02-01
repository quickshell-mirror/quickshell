#pragma once

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

class QtShell: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<QObject> components READ components FINAL);
	Q_CLASSINFO("DefaultProperty", "components");
	QML_ELEMENT;

public:
	explicit QtShell(QObject* parent = nullptr): QObject(parent) {}

	QQmlListProperty<QObject> components();

public slots:
	void reload();

private:
	static void appendComponent(QQmlListProperty<QObject>* list, QObject* component);
};
