#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

#include "scavenge.hpp"

class QtShell: public Scavenger, virtual public Scavengeable {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<QObject> components READ components FINAL);
	Q_CLASSINFO("DefaultProperty", "components");
	QML_ELEMENT;

public:
	explicit QtShell(QObject* parent = nullptr): Scavenger(parent) {}

	void earlyInit(QObject* old) override;
	QObject* scavengeTargetFor(QObject* child) override;

	QQmlListProperty<QObject> components();

private:
	static void appendComponent(QQmlListProperty<QObject>* list, QObject* component);

public:
	// track only the children assigned to `components` in order
	QList<QObject*> children;
	QList<QObject*> scavengeableChildren;
};
