#pragma once

#include <qbytearrayview.h>
#include <qjsonobject.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::hyprland::ipc {

class HyprlandWorkspace: public QObject {
	Q_OBJECT;
	Q_PROPERTY(qint32 id READ id NOTIFY idChanged);
	Q_PROPERTY(QString name READ name NOTIFY nameChanged);
	/// Last json returned for this workspace, as a javascript object.
	///
	/// > [!WARNING] This is *not* updated unless the workspace object is fetched again from
	/// > Hyprland. If you need a value that is subject to change and does not have a dedicated
	/// > property, run @@Hyprland.refreshWorkspaces() and wait for this property to update.
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	Q_PROPERTY(HyprlandMonitor* monitor READ monitor NOTIFY monitorChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("HyprlandWorkspaces must be retrieved from the HyprlandIpc object.");

public:
	explicit HyprlandWorkspace(HyprlandIpc* ipc): QObject(ipc), ipc(ipc) {}

	void updateInitial(qint32 id, QString name);
	void updateFromObject(QVariantMap object);

	[[nodiscard]] qint32 id() const;

	[[nodiscard]] QString name() const;
	void setName(QString name);

	[[nodiscard]] QVariantMap lastIpcObject() const;

	[[nodiscard]] HyprlandMonitor* monitor() const;
	void setMonitor(HyprlandMonitor* monitor);

signals:
	void idChanged();
	void nameChanged();
	void lastIpcObjectChanged();
	void monitorChanged();

private slots:
	void onMonitorDestroyed();

private:
	HyprlandIpc* ipc;

	qint32 mId = -1;
	QString mName;
	QVariantMap mLastIpcObject;
	HyprlandMonitor* mMonitor = nullptr;
};

} // namespace qs::hyprland::ipc
