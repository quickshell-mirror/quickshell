#pragma once

#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qtypes.h>

#include "controller.hpp"

namespace qs::i3::ipc {

///! I3/Sway focused window
class I3Window: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Id for the window
	Q_PROPERTY(qint32 id READ default NOTIFY idChanged BINDABLE bindableId);
	/// Title of the window
	Q_PROPERTY(QString title READ default NOTIFY titleChanged BINDABLE bindableTitle);
	/// Window has mark?
	Q_PROPERTY(bool mark READ default NOTIFY markChanged BINDABLE bindableMark);
	/// Window has trailmark?
	Q_PROPERTY(bool trailmark READ default NOTIFY trailmarkChanged BINDABLE bindableTrailmark);
	/// Last JSON returned for this scroller, as a JavaScript object.
	///
	/// This updates every time we receive a `scroller` event from i3/Sway/Scroll
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("I3Window must be retrieved from the I3 object.");

public:
	I3Window(I3IpcController* ipc);

	[[nodiscard]] QBindable<qint32> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableTitle() { return &this->bTitle; }
	[[nodiscard]] QBindable<bool> bindableMark() { return &this->bMark; }
	[[nodiscard]] QBindable<bool> bindableTrailmark() { return &this->bTrailmark; }
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);

signals:
	void idChanged();
	void titleChanged();
	void markChanged();
	void trailmarkChanged();
	void lastIpcObjectChanged();

private:
	I3IpcController* ipc;

	QVariantMap mLastIpcObject;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Window, qint32, bId, -1, &I3Window::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Window, QString, bTitle, &I3Window::titleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Window, bool, bMark, &I3Window::markChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Window, bool, bTrailmark, &I3Window::trailmarkChanged);
	// clang-format on
};
} // namespace qs::i3::ipc
