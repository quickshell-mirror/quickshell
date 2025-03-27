#pragma once

#include <qobject.h>
#include <qproperty.h>

#include "connection.hpp"

namespace qs::i3::ipc {

///! I3/Sway monitors
class I3Monitor: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The ID of this monitor
	Q_PROPERTY(qint32 id READ default NOTIFY idChanged BINDABLE bindableId);
	/// The name of this monitor
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// Wether this monitor is turned on or not
	Q_PROPERTY(bool power READ default NOTIFY powerChanged BINDABLE bindablePower);
	/// The currently active workspace on this monitor, May be null.
	Q_PROPERTY(qs::i3::ipc::I3Workspace* activeWorkspace READ default NOTIFY activeWorkspaceChanged BINDABLE bindableActiveWorkspace);
	/// Deprecated: See @@activeWorkspace.
	Q_PROPERTY(qs::i3::ipc::I3Workspace* focusedWorkspace READ default NOTIFY activeWorkspaceChanged BINDABLE bindableActiveWorkspace);
	/// The X coordinate of this monitor inside the monitor layout
	Q_PROPERTY(qint32 x READ default NOTIFY xChanged BINDABLE bindableX);
	/// The Y coordinate of this monitor inside the monitor layout
	Q_PROPERTY(qint32 y READ default NOTIFY yChanged BINDABLE bindableY);
	/// The width in pixels of this monitor
	Q_PROPERTY(qint32 width READ default NOTIFY widthChanged BINDABLE bindableWidth);
	/// The height in pixels of this monitor
	Q_PROPERTY(qint32 height READ default NOTIFY heightChanged BINDABLE bindableHeight);
	/// The scaling factor of this monitor, 1 means it runs at native resolution
	Q_PROPERTY(qreal scale READ default NOTIFY scaleChanged BINDABLE bindableScale);
	/// Whether this monitor is currently in focus
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	/// Last JSON returned for this monitor, as a JavaScript object.
	///
	/// This updates every time Quickshell receives an `output` event from i3/Sway
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("I3Monitors must be retrieved from the I3Ipc object.");

public:
	explicit I3Monitor(I3Ipc* ipc);

	[[nodiscard]] QBindable<qint32> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<bool> bindablePower() { return &this->bPower; }
	[[nodiscard]] QBindable<qint32> bindableX() { return &this->bX; }
	[[nodiscard]] QBindable<qint32> bindableY() { return &this->bY; }
	[[nodiscard]] QBindable<qint32> bindableWidth() { return &this->bWidth; }
	[[nodiscard]] QBindable<qint32> bindableHeight() { return &this->bHeight; }
	[[nodiscard]] QBindable<qreal> bindableScale() { return &this->bScale; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }

	[[nodiscard]] QBindable<I3Workspace*> bindableActiveWorkspace() {
		return &this->bActiveWorkspace;
	}

	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);
	void updateInitial(const QString& name);

	void setFocusedWorkspace(I3Workspace* workspace);

signals:
	void idChanged();
	void nameChanged();
	void powerChanged();
	void activeWorkspaceChanged();
	void xChanged();
	void yChanged();
	void widthChanged();
	void heightChanged();
	void scaleChanged();
	void lastIpcObjectChanged();
	void focusedChanged();

private:
	I3Ipc* ipc;

	QVariantMap mLastIpcObject;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Monitor, qint32, bId, -1, &I3Monitor::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, QString, bName, &I3Monitor::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, bool, bPower, &I3Monitor::powerChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, qint32, bX, &I3Monitor::xChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, qint32, bY, &I3Monitor::yChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, qint32, bWidth, &I3Monitor::widthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, qint32, bHeight, &I3Monitor::heightChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Monitor, qreal, bScale, 1, &I3Monitor::scaleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, bool, bFocused, &I3Monitor::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Monitor, I3Workspace*, bActiveWorkspace, &I3Monitor::activeWorkspaceChanged);
	// clang-format on
};

} // namespace qs::i3::ipc
