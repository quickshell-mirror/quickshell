#include "window.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qjsonarray.h>

#include "controller.hpp"

namespace qs::i3::ipc {

I3Window::I3Window(I3IpcController* ipc): QObject(ipc), ipc(ipc) {}

QVariantMap I3Window::lastIpcObject() const { return this->mLastIpcObject; }

void I3Window::updateFromObject(const QVariantMap& obj) {
	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}

	auto container = obj.value("container").toJsonObject();
	auto change = obj.value("change").value<QString>();

	Qt::beginPropertyUpdateGroup();

	if (change == "focus") {
		this->bId = container["id"].toInt();
		this->bTitle = container["name"].toString();
		auto marks = container["marks"];
		this->bMark = !(marks.isNull() || marks.toArray().empty());
		if (ipc->compositor() == "scroll") {
			this->bTrailmark = container["trailmark"].toBool();
		}
	} else if (change == "title") {
		if (container["id"].toInt() == this->bId) {
			this->bTitle = container["name"].toString();
		}
	} else if (change == "close") {
		if (container["id"].toInt() == this->bId) {
			this->bTitle = QString();
		}
	} else if (change == "mark") {
		if (container["id"].toInt() == this->bId) {
			auto marks = container["marks"];
			this->bMark = !(marks.isNull() || marks.toArray().empty());
		}
	} else if (ipc->compositor() == "scroll" && change == "trailmark") {
		if (container["id"].toInt() == this->bId) {
			this->bTrailmark = container["trailmark"].toBool();
		}
	}

	Qt::endPropertyUpdateGroup();
}

} // namespace qs::i3::ipc
