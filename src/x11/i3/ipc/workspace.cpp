#include "workspace.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "controller.hpp"
#include "monitor.hpp"

namespace qs::i3::ipc {

I3Workspace::I3Workspace(I3IpcController* ipc): QObject(ipc), ipc(ipc) {
	Qt::beginPropertyUpdateGroup();

	this->bActive.setBinding([this]() {
		return this->bMonitor.value() && this->bMonitor->bindableActiveWorkspace().value() == this;
	});

	this->bFocused.setBinding([this]() {
		return this->ipc->bindableFocusedWorkspace().value() == this;
	});

	Qt::endPropertyUpdateGroup();
}

QVariantMap I3Workspace::lastIpcObject() const { return this->mLastIpcObject; }

void I3Workspace::updateFromObject(const QVariantMap& obj) {
	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}

	Qt::beginPropertyUpdateGroup();

	this->bId = obj.value("id").value<qint32>();
	this->bName = obj.value("name").value<QString>();
	this->bNumber = obj.value("num").value<qint32>();
	this->bUrgent = obj.value("urgent").value<bool>();

	auto monitorName = obj.value("output").value<QString>();

	auto* monitor = this->bMonitor.value();
	if (!monitor || monitorName != monitor->bindableName().value()) {
		if (monitorName.isEmpty()) {
			monitor = nullptr;
		} else {
			monitor = this->ipc->findMonitorByName(monitorName, true);
		}
	}

	this->setMonitor(monitor);

	Qt::endPropertyUpdateGroup();
}

void I3Workspace::activate() {
	this->ipc->dispatch(QString("workspace number %1").arg(this->bNumber.value()));
}

void I3Workspace::setMonitor(I3Monitor* monitor) {
	auto* oldMonitor = this->bMonitor.value();
	if (oldMonitor == monitor) return;

	if (oldMonitor) {
		QObject::disconnect(oldMonitor, nullptr, this, nullptr);
	}

	if (monitor) {
		QObject::connect(monitor, &QObject::destroyed, this, &I3Workspace::onMonitorDestroyed);
	}

	this->bMonitor = monitor;
}

void I3Workspace::onMonitorDestroyed() { this->bMonitor = nullptr; }

} // namespace qs::i3::ipc
