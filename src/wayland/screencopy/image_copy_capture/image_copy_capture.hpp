#pragma once

#include <qscreen.h>
#include <qwayland-ext-image-capture-source-v1.h>
#include <qwayland-ext-image-copy-capture-v1.h>
#include <qwaylandclientextension.h>

#include "../manager.hpp"

namespace qs::wayland::screencopy::icc {

class IccManager
    : public QWaylandClientExtensionTemplate<IccManager>
    , public QtWayland::ext_image_copy_capture_manager_v1 {
public:
	ScreencopyContext* createSession(::ext_image_capture_source_v1* source, bool paintCursors);

	static IccManager* instance();

private:
	explicit IccManager();
};

class IccOutputSourceManager
    : public QWaylandClientExtensionTemplate<IccOutputSourceManager>
    , public QtWayland::ext_output_image_capture_source_manager_v1 {
public:
	ScreencopyContext* captureOutput(QScreen* screen, bool paintCursors);

	static IccOutputSourceManager* instance();

private:
	explicit IccOutputSourceManager();
};

} // namespace qs::wayland::screencopy::icc
