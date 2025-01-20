#pragma once

#include "../window/proxywindow.hpp"

namespace qs::wayland::util {

void scheduleCommit(ProxyWindowBase* window);

}
