#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../toplevel_management/handle.hpp"
#include "../../toplevel_management/qml.hpp"

namespace qs::hyprland::ipc {

//! Exposes Hyprland window address for a Toplevel
/// Attached object of @@Quickshell.Wayland.Toplevel which exposes
/// a Hyprland window address for the window.
class HyprlandToplevel: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	QML_ATTACHED(HyprlandToplevel);
	/// Hexadecimal Hyprland window address. Will be an empty string until
	/// the address is reported.
	Q_PROPERTY(QString address READ address NOTIFY addressChanged);

public:
	explicit HyprlandToplevel(qs::wayland::toplevel_management::Toplevel* toplevel);

	[[nodiscard]] QString address() { return this->mAddress; }

	static HyprlandToplevel* qmlAttachedProperties(QObject* object);

signals:
	void addressChanged();

private slots:
	void onToplevelAddressed(
	    qs::wayland::toplevel_management::impl::ToplevelHandle* handle,
	    quint64 address
	);

private:
	void setAddress(quint64 address);

	QString mAddress;
	// doesn't have to be nulled on destroy, only used for comparison
	qs::wayland::toplevel_management::impl::ToplevelHandle* handle;
};

} // namespace qs::hyprland::ipc
