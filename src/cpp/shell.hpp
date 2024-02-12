#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

#include "scavenge.hpp"

class ShellConfig {
	Q_GADGET;
	Q_PROPERTY(bool watchFiles MEMBER mWatchFiles);

public:
	bool mWatchFiles = true;
};

///! Root config element
class ShellRoot: public Scavenger, virtual public Scavengeable {
	Q_OBJECT;
	/// If `config.watchFiles` is true the configuration will be reloaded whenever it changes.
	/// Defaults to true.
	Q_PROPERTY(ShellConfig config READ config WRITE setConfig);
	Q_PROPERTY(QQmlListProperty<QObject> components READ components);
	Q_CLASSINFO("DefaultProperty", "components");
	QML_ELEMENT;

public:
	explicit ShellRoot(QObject* parent = nullptr): Scavenger(parent) {}

	void earlyInit(QObject* old) override;
	QObject* scavengeTargetFor(QObject* child) override;

	void setConfig(ShellConfig config);
	[[nodiscard]] ShellConfig config() const;

	QQmlListProperty<QObject> components();

signals:
	void configChanged();

private:
	static void appendComponent(QQmlListProperty<QObject>* list, QObject* component);

	ShellConfig mConfig;

	// track only the children assigned to `components` in order
	QList<QObject*> children;
	QList<QObject*> scavengeableChildren;
};
