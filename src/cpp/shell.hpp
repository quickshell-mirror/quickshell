#pragma once

#include <qdir.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>

extern QString CONFIG_PATH; // NOLINT

class ShellComponent;

class QtShell: public QObject, public QQmlParserStatus {
	Q_OBJECT;
	Q_INTERFACES(QQmlParserStatus);
	Q_PROPERTY(QQmlListProperty<ShellComponent> components READ components FINAL);
	Q_CLASSINFO("DefaultProperty", "components");
	QML_ELEMENT;

public:
	explicit QtShell();

	void classBegin() override {};
	void componentComplete() override;

	QQmlListProperty<ShellComponent> components();

private:
	static void appendComponent(QQmlListProperty<ShellComponent>* list, ShellComponent* component);

	QList<ShellComponent*> mComponents;
	QString path;
	QDir dir;
};

class ShellComponent: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString source WRITE setSource);
	Q_PROPERTY(QQmlComponent* component MEMBER mComponent WRITE setComponent);
	Q_CLASSINFO("DefaultProperty", "component");
	QML_ELEMENT;

public:
	explicit ShellComponent(QObject* parent = nullptr): QObject(parent) {}

	void setSource(QString source);
	void setComponent(QQmlComponent* component);
	void prepare(const QDir& basePath);

signals:
	void ready();

private:
	QString mSource;
	QQmlComponent* mComponent = nullptr;
};
