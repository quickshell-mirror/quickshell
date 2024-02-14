#pragma once

#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qtmetamacros.h>

extern QObject* SCAVENGE_PARENT; // NOLINT

class Scavenger: public QObject, public QQmlParserStatus {
	Q_OBJECT;
	Q_INTERFACES(QQmlParserStatus);

public:
	explicit Scavenger(QObject* parent = nullptr): QObject(parent) {}
	~Scavenger() override = default;

	Scavenger(Scavenger&) = delete;
	Scavenger(Scavenger&&) = delete;
	void operator=(Scavenger&) = delete;
	void operator=(Scavenger&&) = delete;

	void classBegin() override;
	void componentComplete() override {}

protected:
	// do early init, sometimes with a scavengeable target
	virtual void earlyInit(QObject* old) = 0;
};

class Scavengeable {
public:
	explicit Scavengeable() = default;
	virtual ~Scavengeable() = default;

	Scavengeable(Scavengeable&) = delete;
	Scavengeable(Scavengeable&&) = delete;
	void operator=(Scavengeable&) = delete;
	void operator=(Scavengeable&&) = delete;

	// return an old object that might have salvageable resources
	virtual QObject* scavengeTargetFor(QObject* child) = 0;
};

QObject* createComponentScavengeable(
    QObject& parent,
    QQmlComponent& component,
    QVariantMap& initialProperties
);

///! Reloader connection scope
/// Attempts to maintain scavengeable connections.
/// This is mostly useful to split a scavengeable component slot (e.g. `Variants`)
/// into multiple slots.
///
/// If you don't know what that means you probably don't need it.
class ScavengeableScope: public Scavenger, virtual public Scavengeable {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	Q_CLASSINFO("DefaultProperty", "data");
	QML_ELEMENT;

public:
	explicit ScavengeableScope(QObject* parent = nullptr): Scavenger(parent) {}

	void earlyInit(QObject* old) override;
	QObject* scavengeTargetFor(QObject* child) override;

	QQmlListProperty<QObject> data();

private:
	static void appendComponent(QQmlListProperty<QObject>* list, QObject* component);

	// track only the children assigned to `data` in order
	QList<QObject*> mData;
	QList<QObject*> scavengeableData;
};
