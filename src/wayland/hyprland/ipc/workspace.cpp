#include "workspace.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "monitor.hpp"

namespace qs::hyprland::ipc {

qint32 HyprlandWorkspace::id() const { return this->mId; }
QString HyprlandWorkspace::name() const { return this->mName; }
QVariantMap HyprlandWorkspace::lastIpcObject() const { return this->mLastIpcObject; }

void HyprlandWorkspace::updateInitial(qint32 id, QString name) {
	if (id != this->mId) {
		this->mId = id;
		emit this->idChanged();
	}

	if (name != this->mName) {
		this->mName = std::move(name);
		emit this->nameChanged();
	}
}

void HyprlandWorkspace::updateFromObject(QVariantMap object) {
	auto id = object.value("id").value<qint32>();
	auto name = object.value("name").value<QString>();
	auto monitorId = object.value("monitorID").value<qint32>();
	auto monitorName = object.value("monitor").value<QString>();

	if (id != this->mId) {
		this->mId = id;
		emit this->idChanged();
	}

	if (name != this->mName) {
		this->mName = std::move(name);
		emit this->nameChanged();
	}

	if (!monitorName.isEmpty()
	    && (this->mMonitor == nullptr || this->mMonitor->name() != monitorName))
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
