#pragma once

#include <private/qwaylandwindow_p.h>
#include <qtclasshelpermacros.h>
#include <qwayland-idle-inhibit-unstable-v1.h>
#include <qwaylandclientextension.h>

#include "wayland-idle-inhibit-unstable-v1-client-protocol.h"

namespace qs::wayland::idle_inhibit::impl {

class IdleInhibitor;

class IdleInhibitManager
    : public QWaylandClientExtensionTemplate<IdleInhibitManager>
    , public QtWayland::zwp_idle_inhibit_manager_v1 {
public:
	explicit IdleInhibitManager();

	IdleInhibitor* createIdleInhibitor(QtWaylandClient::QWaylandWindow* surface);

	static IdleInhibitManager* instance();
};

class IdleInhibitor: public QtWayland::zwp_idle_inhibitor_v1 {
public:
	explicit IdleInhibitor(::zwp_idle_inhibitor_v1* inhibitor)
	    : QtWayland::zwp_idle_inhibitor_v1(inhibitor) {}

	~IdleInhibitor() override;
	Q_DISABLE_COPY_MOVE(IdleInhibitor);
};

} // namespace qs::wayland::idle_inhibit::impl
