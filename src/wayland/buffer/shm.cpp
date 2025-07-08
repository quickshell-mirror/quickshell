#include "shm.hpp"
#include <algorithm>
#include <memory>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandshm_p.h>
#include <private/qwaylandshmbackingstore_p.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qquickwindow.h>
#include <qsize.h>
#include <wayland-client-protocol.h>

#include "../../core/logcat.hpp"
#include "manager.hpp"

namespace qs::wayland::buffer::shm {

namespace {
QS_LOGGING_CATEGORY(logShm, "quickshell.wayland.buffer.shm", QtWarningMsg);
}

bool WlShmBuffer::isCompatible(const WlBufferRequest& request) const {
	if (QSize(static_cast<int>(request.width), static_cast<int>(request.height)) != this->size()) {
		return false;
	}

	auto matchingFormat = std::ranges::find(request.shm.formats, this->format);
	return matchingFormat != request.shm.formats.end();
}

QDebug& operator<<(QDebug& debug, const WlShmBuffer* buffer) {
	auto saver = QDebugStateSaver(debug);
	debug.nospace();

	if (buffer) {
		auto fmt = QtWaylandClient::QWaylandShm::formatFrom(
		    static_cast<::wl_shm_format>(buffer->format) // NOLINT
		);

		debug << "WlShmBuffer(" << static_cast<const void*>(buffer) << ", size=" << buffer->size()
		      << ", format=" << fmt << ')';
	} else {
		debug << "WlShmBuffer(0x0)";
	}

	return debug;
}

WlShmBuffer::~WlShmBuffer() { qCDebug(logShm) << "Destroyed" << this; }

WlBufferQSGTexture* WlShmBuffer::createQsgTexture(QQuickWindow* window) const {
	auto* texture = new WlShmBufferQSGTexture();

	// If the QWaylandShmBuffer is destroyed before the QSGTexture, we'll hit a UAF
	// in the render thread.
	texture->shmBuffer = this->shmBuffer;

	texture->qsgTexture.reset(window->createTextureFromImage(*this->shmBuffer->image()));
	texture->sync(this, window);
	return texture;
}

void WlShmBufferQSGTexture::sync(const WlBuffer* /*unused*/, QQuickWindow* window) {
	// This is both dumb and expensive. We should use an RHI texture and render images into
	// it more intelligently, but shm buffers are already a horribly slow fallback path,
	// to the point where it barely matters.
	this->qsgTexture.reset(window->createTextureFromImage(*this->shmBuffer->image()));
}

WlBuffer* ShmbufManager::createShmbuf(const WlBufferRequest& request) {
	if (request.shm.formats.isEmpty()) return nullptr;

	static const auto* waylandIntegration = QtWaylandClient::QWaylandIntegration::instance();
	auto* display = waylandIntegration->display();

	// Its probably fine...
	auto format = request.shm.formats[0];

	auto* buffer = new WlShmBuffer(
	    new QtWaylandClient::QWaylandShmBuffer(
	        display,
	        QSize(static_cast<int>(request.width), static_cast<int>(request.height)),
	        QtWaylandClient::QWaylandShm::formatFrom(static_cast<::wl_shm_format>(format)) // NOLINT
	    ),
	    format
	);

	qCDebug(logShm) << "Created shmbuf" << buffer;
	return buffer;
}
} // namespace qs::wayland::buffer::shm
