#include "window.hpp"

#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qproperty.h>

namespace qs::niri::ipc {

void NiriWindow::updateFromJson(const QJsonObject& object, const QString& outputName) {
	auto layout = object.value("layout").toObject();

	Qt::beginPropertyUpdateGroup();
	this->bId = static_cast<qint64>(object.value("id").toDouble(-1));
	this->bTitle = object.value("title").toString();
	this->bAppId = object.value("app_id").toString();
	this->bWorkspaceId = static_cast<qint64>(object.value("workspace_id").toDouble(-1));
	this->bFocused = object.value("is_focused").toBool();
	this->bOutput = outputName;
	this->updatePositionFromLayout(layout);
	Qt::endPropertyUpdateGroup();
}

void NiriWindow::updateLayout(const QJsonObject& layout) {
	Qt::beginPropertyUpdateGroup();
	this->updatePositionFromLayout(layout);
	Qt::endPropertyUpdateGroup();
}

void NiriWindow::setFocused(bool focused) {
	this->bFocused = focused;
}

void NiriWindow::setOutput(const QString& output) {
	this->bOutput = output;
}

void NiriWindow::updatePositionFromLayout(const QJsonObject& layout) {
	auto pos = layout.value("pos_in_scrolling_layout");
	if (pos.isArray()) {
		auto posArray = pos.toArray();
		this->bPositionX = posArray.size() >= 1 ? posArray.at(0).toInt() : FLOATING_POSITION;
		this->bPositionY = posArray.size() >= 2 ? posArray.at(1).toInt() : FLOATING_POSITION;
		this->bIsFloating = false;
	} else {
		this->bPositionX = FLOATING_POSITION;
		this->bPositionY = FLOATING_POSITION;
		this->bIsFloating = true;
	}
}

} // namespace qs::niri::ipc
