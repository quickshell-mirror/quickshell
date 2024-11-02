#pragma once

#include "connection.hpp"

namespace qs::i3::ipc {

///! I3/Sway workspaces
class I3Workspace: public QObject {
	Q_OBJECT;

	/// The ID of this workspace, it is unique for i3/Sway launch
	Q_PROPERTY(qint32 id READ id NOTIFY idChanged);

	/// The name of this workspace
	Q_PROPERTY(QString name READ name NOTIFY nameChanged);

	/// The number of this workspace
	Q_PROPERTY(qint32 num READ num NOTIFY numChanged);

	/// If a window in this workspace has an urgent notification
	Q_PROPERTY(bool urgent READ urgent NOTIFY urgentChanged);

	/// If this workspace is the one currently in focus
	Q_PROPERTY(bool focused READ focused NOTIFY focusedChanged);

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

	[[nodiscard]] qint32 id() const;
	[[nodiscard]] QString name() const;
	[[nodiscard]] qint32 num() const;
	[[nodiscard]] bool urgent() const;
	[[nodiscard]] bool focused() const;
	[[nodiscard]] I3Monitor* monitor() const;
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);
	void setFocus(bool focus);

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

	qint32 mId = -1;
	QString mName;
	qint32 mNum = -1;
	bool mFocused = false;
	bool mUrgent = false;

	QVariantMap mLastIpcObject;
	I3Monitor* mMonitor = nullptr;
	QString mMonitorName;
};
} // namespace qs::i3::ipc
