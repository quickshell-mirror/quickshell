#pragma once

#include <qhash.h>
#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-hyprland-toplevel-mapping-v1.h>
#include <qwaylandclientextension.h>

#include "../../toplevel_management/handle.hpp"
#include "wayland-hyprland-toplevel-mapping-v1-client-protocol.h"

namespace qs::hyprland::ipc {

class HyprlandToplevelMappingHandle: QtWayland::hyprland_toplevel_window_mapping_handle_v1 {
public:
	explicit HyprlandToplevelMappingHandle(
	    qs::wayland::toplevel_management::impl::ToplevelHandle* handle,
	    ::hyprland_toplevel_window_mapping_handle_v1* mapping
	)
	    : QtWayland::hyprland_toplevel_window_mapping_handle_v1(mapping)
	    , handle(handle) {}

	~HyprlandToplevelMappingHandle() override;
	Q_DISABLE_COPY_MOVE(HyprlandToplevelMappingHandle);

protected:
	void hyprland_toplevel_window_mapping_handle_v1_window_address(
	    quint32 addressHi,
	    quint32 addressLo
	) override;

	void hyprland_toplevel_window_mapping_handle_v1_failed() override;

private:
	qs::wayland::toplevel_management::impl::ToplevelHandle* handle;
};

class HyprlandToplevelMappingManager
    : public QWaylandClientExtensionTemplate<HyprlandToplevelMappingManager>
    , public QtWayland::hyprland_toplevel_mapping_manager_v1 {
	Q_OBJECT;

public:
	explicit HyprlandToplevelMappingManager();

	static HyprlandToplevelMappingManager* instance();

	[[nodiscard]] quint64
	getToplevelAddress(qs::wayland::toplevel_management::impl::ToplevelHandle* handle) const;

signals:
	void toplevelAddressed(
	    qs::wayland::toplevel_management::impl::ToplevelHandle* handle,
	    quint64 address
	);

private slots:
	void onToplevelReady(qs::wayland::toplevel_management::impl::ToplevelHandle* handle);
	void onToplevelDestroyed(QObject* object);

private:
	void
	assignAddress(qs::wayland::toplevel_management::impl::ToplevelHandle* handle, quint64 address);
	QHash<wayland::toplevel_management::impl::ToplevelHandle*, quint64> addresses;

	friend class HyprlandToplevelMappingHandle;
};
} // namespace qs::hyprland::ipc
