#pragma once

#include <qscreen.h>
#include <qwayland-wlr-screencopy-unstable-v1.h>
#include <qwaylandclientextension.h>

#include "../manager.hpp"

namespace qs::wayland::screencopy::wlr {

class WlrScreencopyManager
    : public QWaylandClientExtensionTemplate<WlrScreencopyManager>
    , public QtWayland::zwlr_screencopy_manager_v1 {
public:
	ScreencopyContext* captureOutput(QScreen* screen, bool paintCursors, QRect region = QRect());

	static WlrScreencopyManager* instance();

private:
	explicit WlrScreencopyManager();

	friend class WlrScreencopyContext;
};

} // namespace qs::wayland::screencopy::wlr
