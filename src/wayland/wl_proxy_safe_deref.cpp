#include <dlfcn.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <wayland-client-core.h>
#include <wayland-util.h>

#include "../core/logcat.hpp"

namespace {
QS_LOGGING_CATEGORY(logDeref, "quickshell.wayland.safederef", QtWarningMsg);
using wl_proxy_get_listener_t = const void* (*) (wl_proxy*);
wl_proxy_get_listener_t original_wl_proxy_get_listener = nullptr; // NOLINT
} // namespace

extern "C" {
WL_EXPORT const void* wl_proxy_get_listener(struct wl_proxy* proxy) {
	// Avoid null derefs of protocol objects in qtbase.
	// https://qt-project.atlassian.net/browse/QTBUG-145022
	if (!proxy) [[unlikely]] {
		qCCritical(logDeref) << "wl_proxy_get_listener called with a null proxy!";
		return nullptr;
	}

	return original_wl_proxy_get_listener(proxy);
}
}

// NOLINTBEGIN (concurrency-mt-unsafe)
void installWlProxySafeDeref() {
	dlerror(); // clear old errors

	original_wl_proxy_get_listener =
	    reinterpret_cast<wl_proxy_get_listener_t>(dlsym(RTLD_NEXT, "wl_proxy_get_listener"));

	if (auto* error = dlerror()) {
		qCCritical(logDeref) << "Failed to find wl_proxy_get_listener for hooking:" << error;
	} else {
		qCInfo(logDeref) << "Installed wl_proxy_get_listener hook.";
	}
}
// NOLINTEND
