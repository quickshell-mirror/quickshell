#include "workspace.hpp"
#include <algorithm>
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"
#include "monitor.hpp"

namespace qs::hyprland::ipc {

QVariantMap HyprlandWorkspace::lastIpcObject() const { return this->mLastIpcObject; }

HyprlandWorkspace::HyprlandWorkspace(HyprlandIpc* ipc): QObject(ipc), ipc(ipc) {
	Qt::beginPropertyUpdateGroup();

	this->bActive.setBinding([this]() {
		return this->bMonitor.value() && this->bMonitor->bindableActiveWorkspace().value() == this;
	});

	this->bFocused.setBinding([this]() {
		return this->ipc->bindableFocusedWorkspace().value() == this;
	});

	QObject::connect(this, &HyprlandWorkspace::focusedChanged, this, [this]() {
		if (this->bFocused.value()) {
			this->updateUrgent();
		}
	});

	Qt::endPropertyUpdateGroup();
}

void HyprlandWorkspace::updateInitial(qint32 id, const QString& name) {
	Qt::beginPropertyUpdateGroup();
	this->bId = id;
	this->bName = name;
	Qt::endPropertyUpdateGroup();
}

void HyprlandWorkspace::updateFromObject(QVariantMap object) {
	auto monitorId = object.value("monitorID").value<qint32>();
	auto monitorName = object.value("monitor").value<QString>();
	auto hasFullscreen = object.value("hasfullscreen").value<bool>();

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
	    && (this->bMonitor == nullptr || this->bMonitor->bindableName().value() != monitorName))
	{
		auto* monitor = this->ipc->findMonitorByName(monitorName, true, monitorId);
		this->setMonitor(monitor);
	}

	this->bHasFullscreen = hasFullscreen;
	this->mLastIpcObject = std::move(object);
	emit this->lastIpcObjectChanged();
}

void HyprlandWorkspace::setMonitor(HyprlandMonitor* monitor) {
	auto* oldMonitor = this->bMonitor.value();
	if (monitor == oldMonitor) return;

	if (oldMonitor != nullptr) {
		QObject::disconnect(oldMonitor, nullptr, this, nullptr);
	}

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &HyprlandWorkspace::onMonitorDestroyed);
	}

	this->bMonitor = monitor;
}

void HyprlandWorkspace::onMonitorDestroyed() { this->bMonitor = nullptr; }

void HyprlandWorkspace::insertToplevel(HyprlandToplevel* toplevel) {
	if (!toplevel) return;

	const auto& mList = this->mToplevels.valueList();

	if (std::ranges::find(mList, toplevel) != mList.end()) {
		return;
	}

	this->mToplevels.insertObject(toplevel);

	QObject::connect(toplevel, &QObject::destroyed, this, [this, toplevel]() {
		this->removeToplevel(toplevel);
	});

	QObject::connect(
	    toplevel,
	    &HyprlandToplevel::urgentChanged,
	    this,
	    &HyprlandWorkspace::updateUrgent
	);

	this->updateUrgent();
}

void HyprlandWorkspace::removeToplevel(HyprlandToplevel* toplevel) {
	if (!toplevel) return;

	this->mToplevels.removeObject(toplevel);
	emit this->updateUrgent();
	QObject::disconnect(toplevel, nullptr, this, nullptr);
}

// Triggered when there is an update either on the toplevel list, on a toplevel's urgent state
void HyprlandWorkspace::updateUrgent() {
	const auto& mList = this->mToplevels.valueList();

	const bool hasUrgentToplevel = std::ranges::any_of(mList, [&](HyprlandToplevel* toplevel) {
		return toplevel->bindableUrgent().value();
	});

	if (this->bFocused && hasUrgentToplevel) {
		this->clearUrgent();
		return;
	}

	if (hasUrgentToplevel != this->bUrgent.value()) {
		this->bUrgent = hasUrgentToplevel;
	}
}

void HyprlandWorkspace::clearUrgent() {
	this->bUrgent = false;

	// Clear all urgent toplevels
	const auto& mList = this->mToplevels.valueList();
	for (auto* toplevel: mList) {
		toplevel->bindableUrgent().setValue(false);
	}
}

void HyprlandWorkspace::activate() {
	this->ipc->dispatch(QString("workspace %1").arg(this->bName.value()));
}

} // namespace qs::hyprland::ipc
