#include "shell_integration.hpp"

#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>

#include "session_lock.hpp"
#include "surface.hpp"

QtWaylandClient::QWaylandShellSurface*
QSWaylandSessionLockIntegration::createShellSurface(QtWaylandClient::QWaylandWindow* window) {
	auto* lock = LockWindowExtension::get(window->window());
	if (lock == nullptr || lock->surface == nullptr || !lock->surface->isExposed()) {
		qFatal() << "Visibility canary failed. A window with a LockWindowExtension MUST be set to "
		            "visible via LockWindowExtension::setVisible";
	}

	return lock->surface;
}
