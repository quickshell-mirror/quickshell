#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::niri::ipc {

class NiriWorkspace;

class NiriWindow: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Niri's unique window id. Remains constant while the window is open.
	Q_PROPERTY(qint64 id READ default NOTIFY idChanged BINDABLE bindableId);
	/// Title of the window. Empty when unset.
	Q_PROPERTY(QString title READ default NOTIFY titleChanged BINDABLE bindableTitle);
	/// Application ID of the window. Empty when unset.
	Q_PROPERTY(QString appId READ default NOTIFY appIdChanged BINDABLE bindableAppId);
	/// Process ID that created the window, or 0 if unknown.
	Q_PROPERTY(qint32 pid READ default NOTIFY pidChanged BINDABLE bindablePid);
	/// The workspace this window is on. May be null.
	Q_PROPERTY(qs::niri::ipc::NiriWorkspace* workspace READ default NOTIFY workspaceChanged BINDABLE bindableWorkspace);
	/// If this window is currently focused. At most one window is focused at a time.
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	/// If this window is in the floating layout. Otherwise it is tiled.
	Q_PROPERTY(bool floating READ default NOTIFY floatingChanged BINDABLE bindableFloating);
	/// If this window requests attention.
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);
	/// Last json returned for this window, as a javascript object.
	///
	/// > [!WARNING] This is *not* updated for every property change. Niri sends some
	/// > updates, such as layout changes, separately. If you need a value that does not
	/// > have a dedicated property, run @@Niri.refreshWindows() and wait for this
	/// > property to update.
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	/// Position- and size-related properties of the window, as a javascript object.
	/// Updated when niri reports layout changes. Optional properties of the layout
	/// may be absent for some windows.
	Q_PROPERTY(QVariantMap layout READ layout NOTIFY layoutChanged);
	/// Timestamp when the window was most recently focused, as a javascript object
	/// with `secs` and `nanos` fields from the monotonic clock. Empty if never focused.
	Q_PROPERTY(QVariantMap focusTimestamp READ focusTimestamp NOTIFY focusTimestampChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriWindows must be retrieved from the Niri singleton.");

public:
	explicit NiriWindow(NiriIpc* ipc);

	void updateFromObject(const QVariantMap& object);

	/// Focus this window.
	Q_INVOKABLE void activate();

	[[nodiscard]] QBindable<qint64> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableTitle() { return &this->bTitle; }
	[[nodiscard]] QBindable<QString> bindableAppId() { return &this->bAppId; }
	[[nodiscard]] QBindable<qint32> bindablePid() { return &this->bPid; }
	[[nodiscard]] QBindable<NiriWorkspace*> bindableWorkspace() { return &this->bWorkspace; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }
	[[nodiscard]] QBindable<bool> bindableFloating() { return &this->bFloating; }
	[[nodiscard]] QBindable<bool> bindableUrgent() { return &this->bUrgent; }

	[[nodiscard]] QVariantMap lastIpcObject() const;
	[[nodiscard]] QVariantMap layout() const;
	[[nodiscard]] QVariantMap focusTimestamp() const;

	void setWorkspace(NiriWorkspace* workspace);
	void setLayout(const QVariantMap& layout);
	void setFocusTimestamp(const QVariantMap& focusTimestamp);

signals:
	void idChanged();
	void titleChanged();
	void appIdChanged();
	void pidChanged();
	void workspaceChanged();
	void focusedChanged();
	void floatingChanged();
	void urgentChanged();
	void lastIpcObjectChanged();
	void layoutChanged();
	void focusTimestampChanged();

private slots:
	void onWorkspaceDestroyed();

private:
	NiriIpc* ipc;
	QVariantMap mLastIpcObject;
	QVariantMap mLayout;
	QVariantMap mFocusTimestamp;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWindow, qint64, bId, 0, &NiriWindow::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, QString, bTitle, &NiriWindow::titleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, QString, bAppId, &NiriWindow::appIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, qint32, bPid, &NiriWindow::pidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, NiriWorkspace*, bWorkspace, &NiriWindow::workspaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, bool, bFocused, &NiriWindow::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, bool, bFloating, &NiriWindow::floatingChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, bool, bUrgent, &NiriWindow::urgentChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
