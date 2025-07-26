#include "image_copy_capture.hpp"
#include <cstdint>

#include <private/qwaylandscreen_p.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qrect.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qwayland-ext-image-copy-capture-v1.h>
#include <qwaylandclientextension.h>
#include <sys/types.h>
#include <wayland-ext-image-copy-capture-v1-client-protocol.h>
#include <wayland-util.h>

#include "../../../core/logcat.hpp"
#include "../manager.hpp"
#include "image_copy_capture_p.hpp"

namespace qs::wayland::screencopy::icc {

namespace {
QS_LOGGING_CATEGORY(logIcc, "quickshell.wayland.screencopy.icc", QtWarningMsg);
}

using IccCaptureSession = QtWayland::ext_image_copy_capture_session_v1;
using IccCaptureFrame = QtWayland::ext_image_copy_capture_frame_v1;

IccScreencopyContext::IccScreencopyContext(::ext_image_copy_capture_session_v1* session)
    : IccCaptureSession(session) {}

IccScreencopyContext::~IccScreencopyContext() {
	if (this->IccCaptureSession::object()) {
		this->IccCaptureSession::destroy();
	}

	if (this->IccCaptureFrame::object()) {
		this->IccCaptureFrame::destroy();
	}
}

void IccScreencopyContext::captureFrame() {
	if (this->IccCaptureFrame::object() || this->capturePending) return;

	if (this->statePending) this->capturePending = true;
	else this->doCapture();
}

void IccScreencopyContext::ext_image_copy_capture_session_v1_buffer_size(
    uint32_t width,
    uint32_t height
) {
	this->clearOldState();

	this->request.width = width;
	this->request.height = height;
}

void IccScreencopyContext::ext_image_copy_capture_session_v1_shm_format(uint32_t format) {
	this->clearOldState();

	this->request.shm.formats.push(format);
}

void IccScreencopyContext::ext_image_copy_capture_session_v1_dmabuf_device(wl_array* device) {
	this->clearOldState();

	if (device->size != sizeof(dev_t)) {
		qCFatal(logIcc) << "The size of dev_t used by the compositor and quickshell is mismatched. Try "
		                   "recompiling both.";
	}

	this->request.dmabuf.device = *reinterpret_cast<dev_t*>(device->data);
}

void IccScreencopyContext::ext_image_copy_capture_session_v1_dmabuf_format(
    uint32_t format,
    wl_array* modifiers
) {
	this->clearOldState();

	auto* modifierArray = reinterpret_cast<uint64_t*>(modifiers->data);
	auto modifierCount = modifiers->size / sizeof(uint64_t);

	auto reqFormat = buffer::WlBufferRequest::DmaFormat(format);

	for (uint16_t i = 0; i != modifierCount; i++) {
		reqFormat.modifiers.push(modifierArray[i]); // NOLINT
	}

	this->request.dmabuf.formats.push(reqFormat);
}

void IccScreencopyContext::ext_image_copy_capture_session_v1_done() {
	this->statePending = false;

	if (this->capturePending) {
		this->doCapture();
	}
}

void IccScreencopyContext::ext_image_copy_capture_session_v1_stopped() {
	qCInfo(logIcc) << "Ending recording due to screencopy stop for" << this;
	emit this->stopped();
}

void IccScreencopyContext::clearOldState() {
	if (!this->statePending) {
		this->request = buffer::WlBufferRequest();
		this->statePending = true;
	}
}

void IccScreencopyContext::doCapture() {
	this->capturePending = false;

	auto newBuffer = false;
	auto* backbuffer = this->mSwapchain.createBackbuffer(this->request, &newBuffer);

	if (!backbuffer || !backbuffer->buffer()) {
		qCWarning(logIcc) << "Backbuffer creation failed for screencopy. Waiting for updated buffer "
		                     "creation parameters before trying again.";
		return;
	}

	this->IccCaptureFrame::init(this->IccCaptureSession::create_frame());
	this->IccCaptureFrame::attach_buffer(backbuffer->buffer());

	if (newBuffer) {
		// If the buffer was replaced, it will be blank and the compositor needs
		// to repaint the whole thing.
		this->IccCaptureFrame::damage_buffer(
		    0,
		    0,
		    static_cast<int>(this->request.width),
		    static_cast<int>(this->request.height)
		);

		// We don't care about partial damage if the whole buffer was replaced.
		this->lastDamage = QRect();
	} else if (!this->lastDamage.isEmpty()) {
		// If buffers were swapped between the last frame and the current one, request a repaint
		// of the backbuffer in the same places that changes to the frontbuffer were recorded.
		this->IccCaptureFrame::damage_buffer(
		    this->lastDamage.x(),
		    this->lastDamage.y(),
		    this->lastDamage.width(),
		    this->lastDamage.height()
		);

		// We don't need to do this more than once per buffer swap.
		this->lastDamage = QRect();
	}

	this->IccCaptureFrame::capture();
}

void IccScreencopyContext::ext_image_copy_capture_frame_v1_transform(uint32_t transform) {
	this->mSwapchain.backbuffer()->transform = transform;
}

void IccScreencopyContext::ext_image_copy_capture_frame_v1_damage(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height
) {
	this->damage = this->damage.united(QRect(x, y, width, height));
}

void IccScreencopyContext::ext_image_copy_capture_frame_v1_ready() {
	this->IccCaptureFrame::destroy();

	this->mSwapchain.swapBuffers();
	this->lastDamage = this->damage;
	this->damage = QRect();

	emit this->frameCaptured();
}

void IccScreencopyContext::ext_image_copy_capture_frame_v1_failed(uint32_t reason) {
	switch (static_cast<IccCaptureFrame::failure_reason>(reason)) {
	case IccCaptureFrame::failure_reason_buffer_constraints:
		qFatal(logIcc) << "Got a buffer_constraints failure, however the buffer matches the last sent "
		                  "size. There is a bug in quickshell or your compositor.";
		break;
	case IccCaptureFrame::failure_reason_stopped:
		// Handled in the ExtCaptureSession handler.
		break;
	case IccCaptureFrame::failure_reason_unknown:
		qCWarning(logIcc) << "Ending recording due to screencopy failure for" << this;
		emit this->stopped();
		break;
	}
}

IccManager::IccManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }

IccManager* IccManager::instance() {
	static auto* instance = new IccManager();
	return instance;
}

ScreencopyContext*
IccManager::createSession(::ext_image_capture_source_v1* source, bool paintCursors) {
	auto* session = this->create_session(
	    source,
	    paintCursors ? QtWayland::ext_image_copy_capture_manager_v1::options_paint_cursors : 0
	);
	return new IccScreencopyContext(session);
}

IccOutputSourceManager::IccOutputSourceManager(): QWaylandClientExtensionTemplate(1) {
	this->initialize();
}

IccOutputSourceManager* IccOutputSourceManager::instance() {
	static auto* instance = new IccOutputSourceManager();
	return instance;
}

ScreencopyContext* IccOutputSourceManager::captureOutput(QScreen* screen, bool paintCursors) {
	auto* waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle());
	if (!waylandScreen) return nullptr;

	return IccManager::instance()->createSession(
	    this->create_source(waylandScreen->output()),
	    paintCursors
	);
}

} // namespace qs::wayland::screencopy::icc
