#pragma once

#include <qobject.h>

#include "connection.hpp"

namespace qs::i3::ipc {

///! I3/Sway monitors
class I3Monitor: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The ID of this monitor
	Q_PROPERTY(qint32 id READ id NOTIFY idChanged);
	/// The name of this monitor
	Q_PROPERTY(QString name READ name NOTIFY nameChanged);
	/// Wether this monitor is turned on or not
	Q_PROPERTY(bool power READ power NOTIFY powerChanged);
	/// The current workspace
	Q_PROPERTY(qs::i3::ipc::I3Workspace* focusedWorkspace READ focusedWorkspace NOTIFY focusedWorkspaceChanged);
	/// The X coordinate of this monitor inside the monitor layout
	Q_PROPERTY(qint32 x READ x NOTIFY xChanged);
	/// The Y coordinate of this monitor inside the monitor layout
	Q_PROPERTY(qint32 y READ y NOTIFY yChanged);
	/// The width in pixels of this monitor
	Q_PROPERTY(qint32 width READ width NOTIFY widthChanged);
	/// The height in pixels of this monitor
	Q_PROPERTY(qint32 height READ height NOTIFY heightChanged);
	/// The scaling factor of this monitor, 1 means it runs at native resolution
	Q_PROPERTY(qreal scale READ scale NOTIFY scaleChanged);
	/// Whether this monitor is currently in focus
	Q_PROPERTY(bool focused READ focused NOTIFY focusedChanged);
	/// Last JSON returned for this monitor, as a JavaScript object.
	///
	/// This updates every time Quickshell receives an `output` event from i3/Sway
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("I3Monitors must be retrieved from the I3Ipc object.");

public:
	explicit I3Monitor(I3Ipc* ipc): QObject(ipc), ipc(ipc) {}

	[[nodiscard]] qint32 id() const;
	[[nodiscard]] QString name() const;
	[[nodiscard]] bool power() const;
	[[nodiscard]] I3Workspace* focusedWorkspace() const;
	[[nodiscard]] qint32 x() const;
	[[nodiscard]] qint32 y() const;
	[[nodiscard]] qint32 width() const;
	[[nodiscard]] qint32 height() const;
	[[nodiscard]] qreal scale() const;
	[[nodiscard]] bool focused() const;
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);

	void setFocusedWorkspace(I3Workspace* workspace);
	void setFocus(bool focus);
signals:
	void idChanged();
	void nameChanged();
	void powerChanged();
	void focusedWorkspaceChanged();
	void xChanged();
	void yChanged();
	void widthChanged();
	void heightChanged();
	void scaleChanged();
	void lastIpcObjectChanged();
	void focusedChanged();

private:
	I3Ipc* ipc;

	qint32 mId = -1;
	QString mName;
	bool mPower = false;
	qint32 mX = 0;
	qint32 mY = 0;
	qint32 mWidth = 0;
	qint32 mHeight = 0;
	qreal mScale = 1;
	bool mFocused = false;
	QVariantMap mLastIpcObject;

	I3Workspace* mFocusedWorkspace = nullptr;
	QString mFocusedWorkspaceName; // use for faster change detection
};

} // namespace qs::i3::ipc
