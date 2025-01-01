#include "util.hpp"

#include <private/qwaylandwindow_p.h>
#include <qpa/qwindowsysteminterface.h>

namespace qs::wayland::util {

void scheduleCommit(QtWaylandClient::QWaylandWindow* window) {
	// This seems to be one of the less offensive ways to force Qt to send a wl_surface.commit on its own terms.
	// Ideally we would trigger the commit more directly.
	QWindowSystemInterface::handleExposeEvent(
	    window->window(),
	    QRect(QPoint(), window->geometry().size())
	);
}

} // namespace qs::wayland::util
