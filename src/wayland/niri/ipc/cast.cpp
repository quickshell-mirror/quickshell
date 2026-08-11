#include "cast.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtypes.h>
#include <qvariant.h>

#include "connection.hpp"

namespace qs::niri::ipc {

NiriCast::NiriCast(NiriIpc* ipc): QObject(ipc), ipc(ipc) {}

void NiriCast::updateFromObject(const QVariantMap& object) {
	Qt::beginPropertyUpdateGroup();

	// Stream ID cannot be updated after creation.
	if (this->bStreamId.value() == 0) {
		this->bStreamId = object.value("stream_id").value<qint64>();
	}

	this->bSessionId = object.value("session_id").value<qint64>();
	this->bKind = object.value("kind").value<QString>();
	this->bDynamicTarget = object.value("is_dynamic_target").value<bool>();
	this->bActive = object.value("is_active").value<bool>();
	this->bPid = object.value("pid").value<qint32>();
	auto pwNodeId = object.value("pw_node_id");
	this->bPwNodeId = pwNodeId.isNull() ? -1 : pwNodeId.value<qint32>();

	// The target is an enum: Nothing, Output {name}, or Window {id}.
	auto target = object.value("target").toMap();

	if (auto output = target.value("Output").toMap(); !output.isEmpty()) {
		this->bTargetOutput = output.value("name").value<QString>();
		this->bTargetWindowId = 0;
	} else if (auto window = target.value("Window").toMap(); !window.isEmpty()) {
		this->bTargetOutput = QString();
		this->bTargetWindowId = window.value("id").value<qint64>();
	} else {
		this->bTargetOutput = QString();
		this->bTargetWindowId = 0;
	}

	Qt::endPropertyUpdateGroup();
}

} // namespace qs::niri::ipc
