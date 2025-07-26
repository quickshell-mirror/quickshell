#include "hyprland_screencopy.hpp"
#include <cstdint>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwaylandclientextension.h>
#include <wayland-hyprland-toplevel-export-v1-client-protocol.h>

#include "../../../core/logcat.hpp"
#include "../../toplevel_management/handle.hpp"
#include "../manager.hpp"
#include "hyprland_screencopy_p.hpp"

namespace qs::wayland::screencopy::hyprland {

namespace {
QS_LOGGING_CATEGORY(logScreencopy, "quickshell.wayland.screencopy.hyprland", QtWarningMsg);
}

HyprlandScreencopyManager::HyprlandScreencopyManager(): QWaylandClientExtensionTemplate(2) {
	this->initialize();
}

HyprlandScreencopyManager* HyprlandScreencopyManager::instance() {
	static auto* instance = new HyprlandScreencopyManager();
	return instance;
}

ScreencopyContext* HyprlandScreencopyManager::captureToplevel(
    toplevel_management::impl::ToplevelHandle* handle,
    bool paintCursors
) {
	return new HyprlandScreencopyContext(this, handle, paintCursors);
}

HyprlandScreencopyContext::HyprlandScreencopyContext(
    HyprlandScreencopyManager* manager,
    toplevel_management::impl::ToplevelHandle* handle,
    bool paintCursors
)
    : manager(manager)
    , handle(handle)
    , paintCursors(paintCursors) {
	QObject::connect(
	    handle,
	    &QObject::destroyed,
	    this,
	    &HyprlandScreencopyContext::onToplevelDestroyed
	);
}

HyprlandScreencopyContext::~HyprlandScreencopyContext() {
	if (this->object()) this->destroy();
}

void HyprlandScreencopyContext::onToplevelDestroyed() {
	qCWarning(logScreencopy) << "Toplevel destroyed while recording. Stopping" << this;
	if (this->object()) this->destroy();
	emit this->stopped();
}

void HyprlandScreencopyContext::captureFrame() {
	if (this->object()) return;

	this->request.reset();

	this->init(this->manager->capture_toplevel_with_wlr_toplevel_handle(
	    this->paintCursors ? 1 : 0,
	    this->handle->object()
	));
}

void HyprlandScreencopyContext::hyprland_toplevel_export_frame_v1_buffer(
    uint32_t format,
    uint32_t width,
    uint32_t height,
    uint32_t /*stride*/
) {
	// While different sizes can technically be requested, that would be insane.
	this->request.width = width;
	this->request.height = height;
	this->request.shm.formats.push(format);
}

void HyprlandScreencopyContext::hyprland_toplevel_export_frame_v1_linux_dmabuf(
    uint32_t format,
    uint32_t width,
    uint32_t height
) {
	// While different sizes can technically be requested, that would be insane.
	this->request.width = width;
	this->request.height = height;
	this->request.dmabuf.formats.push(format);
}

void HyprlandScreencopyContext::hyprland_toplevel_export_frame_v1_flags(uint32_t flags) {
	if (flags & HYPRLAND_TOPLEVEL_EXPORT_FRAME_V1_FLAGS_Y_INVERT) {
		this->mSwapchain.backbuffer()->transform = buffer::WlBufferTransform::Flipped180;
	}
}

void HyprlandScreencopyContext::hyprland_toplevel_export_frame_v1_buffer_done() {
	auto* backbuffer = this->mSwapchain.createBackbuffer(this->request);

	if (!backbuffer || !backbuffer->buffer()) {
		qCWarning(logScreencopy) << "Backbuffer creation failed for screencopy. Skipping frame.";

		// Try again. This will be spammy if the compositor continuously sends bad frames.
		this->destroy();
		this->captureFrame();
		return;
	}

	this->copy(backbuffer->buffer(), this->copiedFirstFrame ? 0 : 1);
}

void HyprlandScreencopyContext::hyprland_toplevel_export_frame_v1_ready(
    uint32_t /*tvSecHi*/,
    uint32_t /*tvSecLo*/,
    uint32_t /*tvNsec*/
) {
	this->destroy();
	this->copiedFirstFrame = true;
	this->mSwapchain.swapBuffers();
	emit this->frameCaptured();
}

void HyprlandScreencopyContext::hyprland_toplevel_export_frame_v1_failed() {
	qCWarning(logScreencopy) << "Ending recording due to screencopy failure for" << this;
	emit this->stopped();
}

} // namespace qs::wayland::screencopy::hyprland
