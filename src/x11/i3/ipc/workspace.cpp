#include "workspace.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"
#include "monitor.hpp"

namespace qs::i3::ipc {

I3Workspace::I3Workspace(I3Ipc* ipc): QObject(ipc), ipc(ipc) {
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

	if (!this->bMonitor || monitorName != this->bMonitor->bindableName().value()) {
		if (monitorName.isEmpty()) {
			this->bMonitor = nullptr;
		} else {
			this->bMonitor = this->ipc->findMonitorByName(monitorName, true);
		}
	}

	Qt::endPropertyUpdateGroup();
}

void I3Workspace::activate() {
	this->ipc->dispatch(QString("workspace number %1").arg(this->bNumber.value()));
}

} // namespace qs::i3::ipc
