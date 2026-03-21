#include "qml.hpp"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtTypes>
#include <QtWaylandClient/QWaylandClientExtension>
#include <qstringlist.h>

#include "manager.hpp"
#include "output.hpp"

namespace qs::dwl {

DwlIpcQml::DwlIpcQml(QObject* parent): QObject(parent) {
	auto* manager = DwlIpcManager::instance();
	this->manager = manager;

	QObject::connect(manager, &DwlIpcManager::tagCountChanged, this, &DwlIpcQml::tagCountChanged);
	QObject::connect(manager, &DwlIpcManager::layoutsChanged, this, &DwlIpcQml::layoutsChanged);
	QObject::connect(manager, &DwlIpcManager::outputAdded, this, &DwlIpcQml::outputsChanged);
	QObject::connect(manager, &DwlIpcManager::outputRemoved, this, &DwlIpcQml::outputsChanged);
	QObject::connect(
	    manager,
	    &QWaylandClientExtension::activeChanged,
	    this,
	    &DwlIpcQml::availableChanged
	);
}

quint32 DwlIpcQml::tagCount() const { return this->manager->tagCount(); }
// NOLINTNEXTLINE(misc-include-cleaner)
QStringList DwlIpcQml::layouts() const { return this->manager->layouts(); }
QList<DwlIpcOutput*> DwlIpcQml::outputs() const { return this->manager->outputs(); }
bool DwlIpcQml::available() const { return this->manager->isActive(); }

DwlIpcOutput* DwlIpcQml::outputForName(const QString& name) const {
	for (DwlIpcOutput* o: this->manager->outputs())
		if (o->outputName() == name) return o;

	return nullptr;
}

} // namespace qs::dwl
