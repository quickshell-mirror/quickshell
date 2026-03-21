#include "workspace.hpp"

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qproperty.h>

namespace qs::niri::ipc {

void NiriWorkspace::updateFromJson(const QJsonObject& object) {
	auto activeWinId = object.value("active_window_id");

	Qt::beginPropertyUpdateGroup();
	this->bId = static_cast<qint64>(object.value("id").toDouble(-1));
	this->bIdx = object.value("idx").toInt();
	this->bName = object.value("name").toString();
	this->bOutput = object.value("output").toString();
	this->bFocused = object.value("is_focused").toBool();
	this->bActive = object.value("is_active").toBool();
	this->bUrgent = object.value("is_urgent").toBool();
	this->bOccupied = !activeWinId.isNull() && !activeWinId.isUndefined();
	this->bActiveWindowId = activeWinId.isNull() ? -1 : static_cast<qint64>(activeWinId.toDouble(-1));
	Qt::endPropertyUpdateGroup();
}

void NiriWorkspace::setFocused(bool focused) {
	this->bFocused = focused;
}

void NiriWorkspace::setActive(bool active) {
	this->bActive = active;
}

void NiriWorkspace::setActiveWindowId(qint64 id) {
	this->bActiveWindowId = id;
	this->bOccupied = (id >= 0);
}

} // namespace qs::niri::ipc
