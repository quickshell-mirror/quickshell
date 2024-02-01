#pragma once

#include <qobject.h>
#include <qqmlcomponent.h>
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
	Scavengeable() = default;
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
