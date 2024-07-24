#pragma once

#include <private/qwayland-xdg-shell.h>
#include <qwaylandclientextension.h>

namespace qs::wayland::xdg_shell {

// Hack that binds xdg_wm_base twice as QtWaylandXdgShell headers are not exported anywhere.

class XdgWmBase
    : public QWaylandClientExtensionTemplate<XdgWmBase>
    , public QtWayland::xdg_wm_base {
public:
	static XdgWmBase* instance();

private:
	explicit XdgWmBase();
};

} // namespace qs::wayland::xdg_shell
