#pragma once

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::hyprland::ipc {

class HyprlandMonitor: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(qint32 id READ id NOTIFY idChanged);
	Q_PROPERTY(QString name READ name NOTIFY nameChanged);
	Q_PROPERTY(QString description READ description NOTIFY descriptionChanged);
	Q_PROPERTY(qint32 x READ x NOTIFY xChanged);
	Q_PROPERTY(qint32 y READ y NOTIFY yChanged);
	Q_PROPERTY(qint32 width READ width NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height NOTIFY heightChanged);
	Q_PROPERTY(qreal scale READ scale NOTIFY scaleChanged);
	/// Last json returned for this monitor, as a javascript object.
	///
	/// > [!WARNING] This is *not* updated unless the monitor object is fetched again from
	/// > Hyprland. If you need a value that is subject to change and does not have a dedicated
	/// > property, run @@Hyprland.refreshMonitors() and wait for this property to update.
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	/// The currently active workspace on this monitor. May be null.
	Q_PROPERTY(qs::hyprland::ipc::HyprlandWorkspace* activeWorkspace READ activeWorkspace NOTIFY activeWorkspaceChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("HyprlandMonitors must be retrieved from the HyprlandIpc object.");

public:
	explicit HyprlandMonitor(HyprlandIpc* ipc): QObject(ipc), ipc(ipc) {}

	void updateInitial(qint32 id, QString name, QString description);
	void updateFromObject(QVariantMap object);

	[[nodiscard]] qint32 id() const;
	[[nodiscard]] QString name() const;
	[[nodiscard]] QString description() const;
	[[nodiscard]] qint32 x() const;
	[[nodiscard]] qint32 y() const;
	[[nodiscard]] qint32 width() const;
	[[nodiscard]] qint32 height() const;
	[[nodiscard]] qreal scale() const;
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void setActiveWorkspace(HyprlandWorkspace* workspace);
	[[nodiscard]] HyprlandWorkspace* activeWorkspace() const;

signals:
	void idChanged();
	void nameChanged();
	void descriptionChanged();
	void xChanged();
	void yChanged();
	void widthChanged();
	void heightChanged();
	void scaleChanged();
	void lastIpcObjectChanged();
	void activeWorkspaceChanged();

private slots:
	void onActiveWorkspaceDestroyed();

private:
	HyprlandIpc* ipc;

	qint32 mId = -1;
	QString mName;
	QString mDescription;
	qint32 mX = 0;
	qint32 mY = 0;
	qint32 mWidth = 0;
	qint32 mHeight = 0;
	qreal mScale = 0;
	QVariantMap mLastIpcObject;

	HyprlandWorkspace* mActiveWorkspace = nullptr;
};

} // namespace qs::hyprland::ipc
