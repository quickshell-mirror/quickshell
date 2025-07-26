#include "wlr_screencopy.hpp"
#include <cstdint>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandscreen_p.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwaylandclientextension.h>
#include <wayland-wlr-screencopy-unstable-v1-client-protocol.h>

#include "../../../core/logcat.hpp"
#include "../../buffer/manager.hpp"
#include "../manager.hpp"
#include "wlr_screencopy_p.hpp"

namespace qs::wayland::screencopy::wlr {

namespace {
QS_LOGGING_CATEGORY(logScreencopy, "quickshell.wayland.screencopy.wlr", QtWarningMsg);
}

WlrScreencopyManager::WlrScreencopyManager(): QWaylandClientExtensionTemplate(3) {
	this->initialize();
}

WlrScreencopyManager* WlrScreencopyManager::instance() {
	static auto* instance = new WlrScreencopyManager();
	return instance;
}

ScreencopyContext*
WlrScreencopyManager::captureOutput(QScreen* screen, bool paintCursors, QRect region) {
	if (!dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle())) return nullptr;
	return new WlrScreencopyContext(this, screen, paintCursors, region);
}

WlrScreencopyContext::WlrScreencopyContext(
    WlrScreencopyManager* manager,
    QScreen* screen,
    bool paintCursors,
    QRect region
)
    : manager(manager)
    , screen(dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle()))
    , paintCursors(paintCursors)
    , region(region) {
	this->transform.setScreen(this->screen);
	QObject::connect(screen, &QObject::destroyed, this, &WlrScreencopyContext::onScreenDestroyed);
}

WlrScreencopyContext::~WlrScreencopyContext() {
	if (this->object()) this->destroy();
}

void WlrScreencopyContext::onScreenDestroyed() {
	qCWarning(logScreencopy) << "Screen destroyed while recording. Stopping" << this;
	if (this->object()) this->destroy();
	emit this->stopped();
}

void WlrScreencopyContext::captureFrame() {
	if (this->object()) return;

	this->request.reset();

	if (this->region.isEmpty()) {
		this->init(manager->capture_output(this->paintCursors ? 1 : 0, screen->output()));
	} else {
		this->init(manager->capture_output_region(
		    this->paintCursors ? 1 : 0,
		    screen->output(),
		    this->region.x(),
		    this->region.y(),
		    this->region.width(),
		    this->region.height()
		));
	}
}

void WlrScreencopyContext::zwlr_screencopy_frame_v1_buffer(
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

void WlrScreencopyContext::zwlr_screencopy_frame_v1_linux_dmabuf(
    uint32_t format,
    uint32_t width,
    uint32_t height
) {
	// While different sizes can technically be requested, that would be insane.
	this->request.width = width;
	this->request.height = height;
	this->request.dmabuf.formats.push(format);
}

void WlrScreencopyContext::zwlr_screencopy_frame_v1_flags(uint32_t flags) {
	this->yInvert = flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT;
}

void WlrScreencopyContext::zwlr_screencopy_frame_v1_buffer_done() {
	auto* backbuffer = this->mSwapchain.createBackbuffer(this->request);

	if (!backbuffer || !backbuffer->buffer()) {
		qCWarning(logScreencopy) << "Backbuffer creation failed for screencopy. Skipping frame.";

		// Try again. This will be spammy if the compositor continuously sends bad frames.
		this->destroy();
		this->captureFrame();
		return;
	}

	if (this->copiedFirstFrame) {
		this->copy_with_damage(backbuffer->buffer());
	} else {
		this->copy(backbuffer->buffer());
	}
}

void WlrScreencopyContext::zwlr_screencopy_frame_v1_ready(
    uint32_t /*tvSecHi*/,
    uint32_t /*tvSecLo*/,
    uint32_t /*tvNsec*/
) {
	this->submitFrame();
}

void WlrScreencopyContext::zwlr_screencopy_frame_v1_failed() {
	qCWarning(logScreencopy) << "Ending recording due to screencopy failure for" << this;
	emit this->stopped();
}

void WlrScreencopyContext::updateTransform(bool previouslyUnset) {
	if (previouslyUnset && this->copiedFirstFrame) this->submitFrame();
}

void WlrScreencopyContext::submitFrame() {
	this->copiedFirstFrame = true;
	if (this->transform.transform == -1) return;

	auto flipTransform =
	    this->yInvert ? buffer::WlBufferTransform::Flipped180 : buffer::WlBufferTransform::Normal0;

	this->mSwapchain.backbuffer()->transform = this->transform.transform ^ flipTransform;

	this->destroy();
	this->mSwapchain.swapBuffers();
	emit this->frameCaptured();
}

WlrScreencopyContext::OutputTransformQuery::OutputTransformQuery(WlrScreencopyContext* context)
    : context(context) {}

WlrScreencopyContext::OutputTransformQuery::~OutputTransformQuery() {
	if (this->isInitialized()) this->release();
}

void WlrScreencopyContext::OutputTransformQuery::setScreen(QtWaylandClient::QWaylandScreen* screen
) {
	// cursed hack
	class QWaylandScreenReflector: public QtWaylandClient::QWaylandScreen {
	public:
		[[nodiscard]] int globalId() const { return this->m_outputId; }
	};

	if (this->isInitialized()) this->release();

	this->init(
	    screen->display()->wl_registry(),
	    static_cast<QWaylandScreenReflector*>(screen)->globalId(), // NOLINT
	    3
	);
}

void WlrScreencopyContext::OutputTransformQuery::output_geometry(
    qint32 /*x*/,
    qint32 /*y*/,
    qint32 /*width*/,
    qint32 /*height*/,
    qint32 /*subpixel*/,
    const QString& /*make*/,
    const QString& /*model*/,
    qint32 transform
) {
	auto newTransform = this->transform == -1;
	this->transform = transform;
	this->context->updateTransform(newTransform);
}

} // namespace qs::wayland::screencopy::wlr
