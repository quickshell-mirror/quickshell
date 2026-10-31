#include "rendercontrol.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qpoint.h>
#include <qquickwindow.h>
#include <qsgrendererinterface.h>
#include <qwindow.h>

#include "../core/logcat.hpp"

namespace qs::window {

namespace {
QS_LOGGING_CATEGORY(logRenderControl, "quickshell.window.rendercontrol", QtWarningMsg);
WindowRenderControl* (*FACTORY)() = nullptr; // NOLINT
bool ENABLED = false;                        // NOLINT
} // namespace

void WindowRenderControl::setEnabled(bool enabled) { ENABLED = enabled; }
void WindowRenderControl::setFactory(WindowRenderControl* (*factory)()) { FACTORY = factory; }

WindowRenderControl* WindowRenderControl::create() {
	if (!ENABLED) return nullptr;

	if (!FACTORY) {
		qCInfo(logRenderControl) << "Render control is unavailable on this platform.";
		return nullptr;
	}

	if (QQuickWindow::graphicsApi() != QSGRendererInterface::OpenGL) {
		qCInfo(logRenderControl) << "Render control requires OpenGL.";
		return nullptr;
	}

	return FACTORY();
}

// Use the window's scale; see QQuickWindow::effectiveDevicePixelRatio().
QWindow* WindowRenderControl::renderWindow(QPoint* offset) {
	if (offset) *offset = QPoint();
	return this->window();
}

} // namespace qs::window
