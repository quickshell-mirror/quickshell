#include "xdgshell.hpp"

#include <qwaylandclientextension.h>

namespace qs::wayland::xdg_shell {

XdgWmBase::XdgWmBase(): QWaylandClientExtensionTemplate(6) { this->initialize(); }

XdgWmBase* XdgWmBase::instance() {
	static auto* instance = new XdgWmBase(); // NOLINT
	return instance;
}

} // namespace qs::wayland::xdg_shell
