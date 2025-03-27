#pragma once

#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::i3::ipc {

class I3Monitor;

///! I3/Sway workspaces
class I3Workspace: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The ID of this workspace, it is unique for i3/Sway launch
	Q_PROPERTY(qint32 id READ default NOTIFY idChanged BINDABLE bindableId);
	/// The name of this workspace
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// The number of this workspace
	Q_PROPERTY(qint32 number READ default NOTIFY numberChanged BINDABLE bindableNumber);
	/// Deprecated: use @@number
	Q_PROPERTY(qint32 num READ default NOTIFY numberChanged BINDABLE bindableNumber);
	/// If a window in this workspace has an urgent notification
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);
	/// If this workspace is currently active on its monitor. See also @@focused.
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	/// If this workspace is currently active on a monitor and that monitor is currently
	/// focused. See also @@active.
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	/// The monitor this workspace is being displayed on
	Q_PROPERTY(qs::i3::ipc::I3Monitor* monitor READ default NOTIFY monitorChanged BINDABLE bindableMonitor);
	/// Last JSON returned for this workspace, as a JavaScript object.
	///
	/// This updates every time we receive a `workspace` event from i3/Sway
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("I3Workspaces must be retrieved from the I3 object.");

public:
	I3Workspace(I3Ipc* ipc);

	/// Activate the workspace.
	///
	/// > [!NOTE] This is equivalent to running
	/// > ```qml
	/// > I3.dispatch(`workspace number ${workspace.number}`);
	/// > ```
	Q_INVOKABLE void activate();

	[[nodiscard]] QBindable<qint32> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<qint32> bindableNumber() { return &this->bNumber; }
	[[nodiscard]] QBindable<bool> bindableActive() { return &this->bActive; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }
	[[nodiscard]] QBindable<bool> bindableUrgent() { return &this->bUrgent; }
	[[nodiscard]] QBindable<I3Monitor*> bindableMonitor() { return &this->bMonitor; }
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);

signals:
	void idChanged();
	void nameChanged();
	void urgentChanged();
	void activeChanged();
	void focusedChanged();
	void numberChanged();
	void monitorChanged();
	void lastIpcObjectChanged();

private:
	I3Ipc* ipc;

	QVariantMap mLastIpcObject;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Workspace, qint32, bId, -1, &I3Workspace::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, QString, bName, &I3Workspace::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Workspace, qint32, bNumber, -1, &I3Workspace::numberChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, bool, bActive, &I3Workspace::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, bool, bFocused, &I3Workspace::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, bool, bUrgent, &I3Workspace::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, I3Monitor*, bMonitor, &I3Workspace::monitorChanged);
	// clang-format on
};
} // namespace qs::i3::ipc
