#include "workspace.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtypes.h>
#include <qvariant.h>

#include "connection.hpp"
#include "monitor.hpp"

namespace qs::niri::ipc {

NiriWorkspace::NiriWorkspace(NiriIpc* ipc): QObject(ipc), ipc(ipc) {
	this->bFocused.setBinding([this]() {
		return this->ipc->bindableFocusedWorkspace().value() == this;
	});
}

void NiriWorkspace::updateFromObject(const QVariantMap& object) {
	Qt::beginPropertyUpdateGroup();

	// ID cannot be updated after creation.
	if (this->bId.value() == 0) {
		this->bId = object.value("id").value<qint64>();
	}

	this->bIdx = object.value("idx").value<qint32>();
	this->bName = object.value("name").value<QString>();
	this->bActive = object.value("is_active").value<bool>();
	this->bUrgent = object.value("is_urgent").value<bool>();
	this->bActiveWindowId = object.value("active_window_id").value<qint64>();

	// output is null when no outputs are connected.
	auto outputName = object.value("output").value<QString>();
	if (outputName.isEmpty()) {
		this->setMonitor(nullptr);
	} else if (this->bMonitor == nullptr
	           || this->bMonitor->bindableName().value() != outputName)
	{
		auto* monitor = this->ipc->findMonitorByName(outputName, true);
		this->setMonitor(monitor);
	}

	Qt::endPropertyUpdateGroup();
}

void NiriWorkspace::setMonitor(NiriMonitor* monitor) {
	auto* oldMonitor = this->bMonitor.value();
	if (monitor == oldMonitor) return;

	if (oldMonitor != nullptr) {
		QObject::disconnect(oldMonitor, nullptr, this, nullptr);
	}

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &NiriWorkspace::onMonitorDestroyed);
	}

	this->bMonitor = monitor;
}

void NiriWorkspace::onMonitorDestroyed() { this->bMonitor = nullptr; }

void NiriWorkspace::activate() {
	this->ipc->dispatch(
	    QString(R"({"FocusWorkspace":{"reference":{"Id":%1}}})").arg(this->bId.value())
	);
}

} // namespace qs::niri::ipc
