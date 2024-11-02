#include "workspace.hpp"

#include <qcontainerfwd.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "monitor.hpp"

namespace qs::i3::ipc {

qint32 I3Workspace ::id() const { return this->mId; }
QString I3Workspace::name() const { return this->mName; }
qint32 I3Workspace ::num() const { return this->mNum; }
bool I3Workspace ::urgent() const { return this->mUrgent; }
bool I3Workspace::focused() const { return this->mFocused; }
I3Monitor* I3Workspace::monitor() const { return this->mMonitor; }
QVariantMap I3Workspace::lastIpcObject() const { return this->mLastIpcObject; }

void I3Workspace::updateFromObject(const QVariantMap& obj) {
	auto id = obj.value("id").value<qint32>();
	auto name = obj.value("name").value<QString>();
	auto num = obj.value("num").value<qint32>();
	auto urgent = obj.value("urgent").value<bool>();
	auto focused = obj.value("focused").value<bool>();
	auto monitorName = obj.value("output").value<QString>();

	if (id != this->mId) {
		this->mId = id;
		emit this->idChanged();
	}

	if (name != this->mName) {
		this->mName = name;
		emit this->nameChanged();
	}

	if (num != this->mNum) {
		this->mNum = num;
		emit this->numChanged();
	}

	if (urgent != this->mUrgent) {
		this->mUrgent = urgent;
		emit this->urgentChanged();
	}

	if (focused != this->mFocused) {
		this->mFocused = focused;
		emit this->focusedChanged();
	}

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

void I3Workspace::setFocus(bool focus) { this->mFocused = focus; }

} // namespace qs::i3::ipc
