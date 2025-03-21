#pragma once

#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::i3::ipc {

///! I3/Sway workspaces
class I3Workspace: public QObject {
	Q_OBJECT;

	/// The ID of this workspace, it is unique for i3/Sway launch
	Q_PROPERTY(qint32 id READ default NOTIFY idChanged BINDABLE bindableId);

	/// The name of this workspace
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);

	/// The number of this workspace
	Q_PROPERTY(qint32 num READ default NOTIFY numChanged BINDABLE bindableNum);

	/// If a window in this workspace has an urgent notification
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);

	/// If this workspace is the one currently in focus
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);

	/// The monitor this workspace is being displayed on
	Q_PROPERTY(qs::i3::ipc::I3Monitor* monitor READ monitor NOTIFY monitorChanged);

	/// Last JSON returned for this workspace, as a JavaScript object.
	///
	/// This updates every time we receive a `workspace` event from i3/Sway
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);

	QML_ELEMENT;
	QML_UNCREATABLE("I3Workspaces must be retrieved from the I3 object.");

public:
	I3Workspace(qs::i3::ipc::I3Ipc* ipc): QObject(ipc), ipc(ipc) {}

	[[nodiscard]] QBindable<qint32> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<qint32> bindableNum() { return &this->bNum; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }
	[[nodiscard]] QBindable<bool> bindableUrgent() { return &this->bUrgent; }
	[[nodiscard]] I3Monitor* monitor() const;
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);

signals:
	void idChanged();
	void nameChanged();
	void urgentChanged();
	void focusedChanged();
	void numChanged();
	void monitorChanged();
	void lastIpcObjectChanged();

private:
	I3Ipc* ipc;

	QVariantMap mLastIpcObject;
	I3Monitor* mMonitor = nullptr;
	QString mMonitorName;

	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Workspace, qint32, bId, -1, &I3Workspace::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, QString, bName, &I3Workspace::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Workspace, qint32, bNum, -1, &I3Workspace::numChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, bool, bFocused, &I3Workspace::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Workspace, bool, bUrgent, &I3Workspace::urgentChanged);
};
} // namespace qs::i3::ipc
