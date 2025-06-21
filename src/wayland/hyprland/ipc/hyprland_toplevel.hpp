#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../toplevel_management/handle.hpp"
#include "../../toplevel_management/qml.hpp"
#include "connection.hpp"

namespace qs::hyprland::ipc {

//! Hyprland Toplevel
/// Represents a window as Hyprland exposes it.
/// Can also be used as an attached object of a @@Quickshell.Wayland.Toplevel,
/// to resolve a handle to an Hyprland toplevel.
class HyprlandToplevel: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	QML_ATTACHED(HyprlandToplevel);
	// clang-format off
	/// Hexadecimal Hyprland window address. Will be an empty string until
	/// the address is reported.
	Q_PROPERTY(QString address READ addressStr NOTIFY addressChanged);
	/// The toplevel handle, exposing the Hyprland toplevel.
	/// Will be null until the address is reported
	Q_PROPERTY(HyprlandToplevel* handle READ hyprlandHandle NOTIFY hyprlandHandleChanged);
	/// The wayland toplevel handle. Will be null intil the address is reported
	Q_PROPERTY(qs::wayland::toplevel_management::Toplevel* wayland READ waylandHandle NOTIFY waylandHandleChanged);
	/// The title of the toplevel
	Q_PROPERTY(QString title READ default NOTIFY titleChanged BINDABLE bindableTitle);
	/// Whether the toplevel is active or not
	Q_PROPERTY(bool activated READ default NOTIFY activatedChanged BINDABLE bindableActivated);
	/// Whether the client is urgent or not
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);
	/// Last json returned for this toplevel, as a javascript object.
	///
	/// > [!WARNING] This is *not* updated unless the toplevel object is fetched again from
	/// > Hyprland. If you need a value that is subject to change and does not have a dedicated
	/// > property, run @@Hyprland.refreshToplevels() and wait for this property to update.
	Q_PROPERTY(QVariantMap lastIpcObject READ default BINDABLE bindableLastIpcObject NOTIFY lastIpcObjectChanged);
	/// The current workspace of the toplevel (might be null)
	Q_PROPERTY(qs::hyprland::ipc::HyprlandWorkspace* workspace READ default NOTIFY workspaceChanged BINDABLE bindableWorkspace);
	/// The current monitor of the toplevel (might be null)
	Q_PROPERTY(qs::hyprland::ipc::HyprlandMonitor* monitor READ default NOTIFY monitorChanged BINDABLE bindableMonitor);
	// clang-format on

public:
	/// When invoked from HyprlandIpc, reacting to Hyprland's IPC events.
	explicit HyprlandToplevel(HyprlandIpc* ipc);
	/// When attached from a Toplevel
	explicit HyprlandToplevel(HyprlandIpc* ipc, qs::wayland::toplevel_management::Toplevel* toplevel);

	static HyprlandToplevel* qmlAttachedProperties(QObject* object);

	void updateInitial(quint64 address, const QString& title, const QString& workspaceName);

	void updateFromObject(const QVariantMap& object);

	[[nodiscard]] QString addressStr() const { return QString::number(this->mAddress, 16); }
	[[nodiscard]] quint64 address() const { return this->mAddress; }
	void setAddress(quint64 address);

	// clang-format off
	[[nodiscard]] HyprlandToplevel* hyprlandHandle() { return this->mHyprlandHandle; }
	void setHyprlandHandle(HyprlandToplevel* handle);

	[[nodiscard]] wayland::toplevel_management::Toplevel* waylandHandle();
	void setWaylandHandle(wayland::toplevel_management::impl::ToplevelHandle* handle);
	// clang-format on

	[[nodiscard]] QBindable<QString> bindableTitle() { return &this->bTitle; }
	[[nodiscard]] QBindable<bool> bindableActivated() { return &this->bActivated; }
	[[nodiscard]] QBindable<bool> bindableUrgent() { return &this->bUrgent; }

	[[nodiscard]] QBindable<QVariantMap> bindableLastIpcObject() const {
		return &this->bLastIpcObject;
	};

	[[nodiscard]] QBindable<HyprlandWorkspace*> bindableWorkspace() { return &this->bWorkspace; }
	void setWorkspace(HyprlandWorkspace* workspace);

	[[nodiscard]] QBindable<HyprlandMonitor*> bindableMonitor() { return &this->bMonitor; }

signals:
	void addressChanged();
	QSDOC_HIDE void waylandHandleChanged();
	QSDOC_HIDE void hyprlandHandleChanged();

	void titleChanged();
	void activatedChanged();
	void urgentChanged();
	void workspaceChanged();
	void monitorChanged();
	void lastIpcObjectChanged();

private slots:
	void onActivatedChanged();

private:
	quint64 mAddress = 0;
	HyprlandIpc* ipc;

	qs::wayland::toplevel_management::impl::ToplevelHandle* mWaylandHandle = nullptr;
	HyprlandToplevel* mHyprlandHandle = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandToplevel, QString, bTitle, &HyprlandToplevel::titleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandToplevel, bool, bActivated, &HyprlandToplevel::activatedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandToplevel, bool, bUrgent, &HyprlandToplevel::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandToplevel, HyprlandWorkspace*, bWorkspace, &HyprlandToplevel::workspaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandToplevel, HyprlandMonitor*, bMonitor, &HyprlandToplevel::monitorChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandToplevel, QVariantMap, bLastIpcObject, &HyprlandToplevel::lastIpcObjectChanged);
	// clang-format on
};

} // namespace qs::hyprland::ipc
