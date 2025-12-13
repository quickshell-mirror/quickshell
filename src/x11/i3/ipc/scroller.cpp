#include "scroller.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "controller.hpp"

namespace qs::i3::ipc {

I3Scroller::I3Scroller(I3IpcController* ipc): QObject(ipc), ipc(ipc) {}

QVariantMap I3Scroller::lastIpcObject() const { return this->mLastIpcObject; }

void I3Scroller::updateFromObject(const QVariantMap& obj) {
	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}

	Qt::beginPropertyUpdateGroup();

	auto workspace = obj.value("workspace").value<QString>();
	this->bWorkspace = this->ipc->findWorkspaceByName(workspace);
	this->bOverview = obj.value("overview").value<bool>();
	this->bScaled = obj.value("scaled").value<bool>();
	this->bScale = obj.value("scale").value<double>();
	this->bMode = obj.value("mode").value<QString>();
	this->bInsert = obj.value("insert").value<QString>();
	this->bFocus = obj.value("focus").value<bool>();
	this->bCenterHorizontal = obj.value("center_horizontal").value<bool>();
	this->bCenterVertical = obj.value("center_vertical").value<bool>();
	this->bReorder = obj.value("reorder").value<QString>();

	Qt::endPropertyUpdateGroup();
}

void I3Scroller::setMode(const QString &mode) {
	this->ipc->dispatch(QString("set_mode %1").arg(mode == "horizontal" ? "h" : "v"));
}

void I3Scroller::setInsert(const QString &insert) {
	this->ipc->dispatch(QString("set_mode %1").arg(insert));
}

void I3Scroller::setFocus(bool focus) {
	this->ipc->dispatch(QString("set_mode %1").arg(focus ? "focus" : "nofocus"));
}

void I3Scroller::setCenterHorizontal(bool center) {
	this->ipc->dispatch(QString("set_mode %1").arg(center ? "center_horiz" : "nocenter_horiz"));
}

void I3Scroller::setCenterVertical(bool center) {
	this->ipc->dispatch(QString("set_mode %1").arg(center ? "center_vert" : "nocenter_vert"));
}

void I3Scroller::setReorder(const QString &reorder) {
	this->ipc->dispatch(QString("set_mode %1").arg(reorder == "auto" ? "reorder_auto" : "noreorder_auto"));
}
	
} // namespace qs::i3::ipc
