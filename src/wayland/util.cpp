#include "util.hpp"

#include "../window/proxywindow.hpp"

namespace qs::wayland::util {

void scheduleCommit(ProxyWindowBase* window) { window->schedulePolish(); }

} // namespace qs::wayland::util
