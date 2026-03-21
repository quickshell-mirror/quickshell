#include "output.hpp"

#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qproperty.h>

namespace qs::niri::ipc {

void NiriOutput::updateFromJson(const QJsonObject& object) {
	auto logical = object.value("logical").toObject();
	auto physicalSize = object.value("physical_size").toArray();
	auto modes = object.value("modes").toArray();
	auto currentModeIdx = object.value("current_mode");
	auto isConnected = !logical.isEmpty() && !currentModeIdx.isNull();

	qreal refreshRate = 0.0;
	if (!currentModeIdx.isNull() && currentModeIdx.toInt(-1) >= 0) {
		auto modeIdx = currentModeIdx.toInt();
		if (modeIdx < modes.size()) {
			auto mode = modes.at(modeIdx).toObject();
			refreshRate = mode.value("refresh_rate").toDouble();
		}
	}

	Qt::beginPropertyUpdateGroup();
	this->bName = object.value("name").toString();
	this->bConnected = isConnected;
	this->bScale = logical.value("scale").toDouble(1.0);
	this->bWidth = logical.value("width").toInt();
	this->bHeight = logical.value("height").toInt();
	this->bX = logical.value("x").toInt();
	this->bY = logical.value("y").toInt();
	this->bPhysicalWidth = physicalSize.size() >= 1 ? physicalSize.at(0).toInt() : 0;
	this->bPhysicalHeight = physicalSize.size() >= 2 ? physicalSize.at(1).toInt() : 0;
	this->bRefreshRate = refreshRate;
	this->bVrrSupported = object.value("vrr_supported").toBool();
	this->bVrrEnabled = object.value("vrr_enabled").toBool();
	this->bTransform = logical.value("transform").toString("Normal");
	Qt::endPropertyUpdateGroup();
}

} // namespace qs::niri::ipc
