#include "workspace.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "monitor.hpp"

namespace qs::hyprland::ipc {

QVariantMap HyprlandWorkspace::lastIpcObject() const { return this->mLastIpcObject; }

void HyprlandWorkspace::updateInitial(qint32 id, const QString& name) {
	Qt::beginPropertyUpdateGroup();
	this->bId = id;
	this->bName = name;
	Qt::endPropertyUpdateGroup();
}

void HyprlandWorkspace::updateFromObject(QVariantMap object) {
	auto monitorId = object.value("monitorID").value<qint32>();
	auto monitorName = object.value("monitor").value<QString>();

	auto initial = this->bId == -1;

	// ID cannot be updated after creation
	if (initial) {
		this->bId = object.value("id").value<qint32>();
	}

	// No events we currently handle give a workspace id but not a name,
	// so we shouldn't set this if it isn't an initial query
	if (initial) {
		this->bName = object.value("name").value<QString>();
	}

	if (!monitorName.isEmpty()
	    && (this->mMonitor == nullptr || this->mMonitor->bindableName().value() != monitorName))
	{
		auto* monitor = this->ipc->findMonitorByName(monitorName, true, monitorId);
		this->setMonitor(monitor);
	}

	this->mLastIpcObject = std::move(object);
	emit this->lastIpcObjectChanged();
}

HyprlandMonitor* HyprlandWorkspace::monitor() const { return this->mMonitor; }

void HyprlandWorkspace::setMonitor(HyprlandMonitor* monitor) {
	if (monitor == this->mMonitor) return;

	if (this->mMonitor != nullptr) {
		QObject::disconnect(this->mMonitor, nullptr, this, nullptr);
	}

	this->mMonitor = monitor;

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &HyprlandWorkspace::onMonitorDestroyed);
	}

	emit this->monitorChanged();
}

void HyprlandWorkspace::onMonitorDestroyed() {
	this->mMonitor = nullptr;
	emit this->monitorChanged();
}

} // namespace qs::hyprland::ipc
