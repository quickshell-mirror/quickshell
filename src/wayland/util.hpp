#pragma once

#include <private/qwaylandwindow_p.h>

namespace qs::wayland::util {

void scheduleCommit(QtWaylandClient::QWaylandWindow* window);

}
