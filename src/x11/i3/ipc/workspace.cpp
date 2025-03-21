#include "workspace.hpp"

#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "monitor.hpp"

namespace qs::i3::ipc {

I3Monitor* I3Workspace::monitor() const { return this->mMonitor; }
QVariantMap I3Workspace::lastIpcObject() const { return this->mLastIpcObject; }

void I3Workspace::updateFromObject(const QVariantMap& obj) {
	Qt::beginPropertyUpdateGroup();
	this->bId = obj.value("id").value<qint32>();
	this->bName = obj.value("name").value<QString>();
	this->bNum = obj.value("num").value<qint32>();
	this->bUrgent = obj.value("urgent").value<bool>();
	this->bFocused = obj.value("focused").value<bool>();
	Qt::endPropertyUpdateGroup();

	auto monitorName = obj.value("output").value<QString>();

	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}

	if (monitorName != this->mMonitorName) {
		auto* monitor = this->ipc->findMonitorByName(monitorName);
		if (monitorName.isEmpty() || monitor == nullptr) { // is null when output is disabled
			this->mMonitor = nullptr;
			this->mMonitorName = "";
		} else {
			this->mMonitorName = monitorName;
			this->mMonitor = monitor;
		}
		emit this->monitorChanged();
	}
}

} // namespace qs::i3::ipc
