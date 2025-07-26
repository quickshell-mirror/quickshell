#include "manager.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmatrix4x4.h>
#include <qnamespace.h>
#include <qquickwindow.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qvectornd.h>

#include "../../core/logcat.hpp"
#include "dmabuf.hpp"
#include "manager_p.hpp"
#include "qsg.hpp"
#include "shm.hpp"

namespace qs::wayland::buffer {

namespace {
QS_LOGGING_CATEGORY(logBuffer, "quickshell.wayland.buffer", QtWarningMsg);
}

void WlBufferRequest::reset() { *this = WlBufferRequest(); }

WlBuffer* WlBufferSwapchain::createBackbuffer(const WlBufferRequest& request, bool* newBuffer) {
	auto& buffer = this->presentSecondBuffer ? this->buffer1 : this->buffer2;

	if (!buffer || !buffer->isCompatible(request)) {
		buffer.reset(WlBufferManager::instance()->createBuffer(request));
		if (newBuffer) *newBuffer = true;
	}

	return buffer.get();
}

WlBufferManager::WlBufferManager(): p(new WlBufferManagerPrivate(this)) {}

WlBufferManager::~WlBufferManager() { delete this->p; }

WlBufferManager* WlBufferManager::instance() {
	static auto* instance = new WlBufferManager();
	return instance;
}

bool WlBufferManager::isReady() const { return this->p->mReady; }

[[nodiscard]] WlBuffer* WlBufferManager::createBuffer(const WlBufferRequest& request) {
	static const bool dmabufDisabled = qEnvironmentVariableIsSet("QS_DISABLE_DMABUF");

	qCDebug(logBuffer).nospace() << "Creating buffer from request at " << request.width << 'x'
	                             << request.height;
	qCDebug(logBuffer).nospace() << "  Dmabuf requests on device " << request.dmabuf.device
	                             << " (disabled: " << dmabufDisabled << ')';

	for (const auto& [format, modifiers]: request.dmabuf.formats) {
		qCDebug(logBuffer).nospace() << "    Format " << dmabuf::FourCCStr(format)
		                             << (modifiers.length() == 0 ? " (No modifiers specified)" : "");

		for (const auto& modifier: modifiers) {
			qCDebug(logBuffer) << "      Explicit Modifier" << dmabuf::FourCCModStr(modifier);
		}
	}

	qCDebug(logBuffer).nospace() << "  Shm requests";

	for (const auto& format: request.shm.formats) {
		qCDebug(logBuffer) << "    Format" << format;
	}

	if (request.width == 0 || request.height == 0) {
		qCWarning(logBuffer) << "Cannot create zero-sized buffer.";
		return nullptr;
	}

	if (!dmabufDisabled) {
		if (auto* buf = this->p->dmabuf.createDmabuf(request)) return buf;
		qCWarning(logBuffer) << "DMA buffer creation failed, falling back to SHM.";
	}

	return shm::ShmbufManager::createShmbuf(request);
}

WlBufferManagerPrivate::WlBufferManagerPrivate(WlBufferManager* manager)
    : manager(manager)
    , dmabuf(this) {}

void WlBufferManagerPrivate::dmabufReady() {
	this->mReady = true;
	emit this->manager->ready();
}

WlBufferQSGDisplayNode::WlBufferQSGDisplayNode(QQuickWindow* window)
    : window(window)
    , imageNode(window->createImageNode()) {
	this->appendChildNode(this->imageNode);
}

void WlBufferQSGDisplayNode::setRect(const QRectF& rect) {
	const auto* buffer = (this->presentSecondBuffer ? this->buffer2 : this->buffer1).first;
	if (!buffer) return;

	auto matrix = QMatrix4x4();
	auto center = rect.center();
	auto centerX = static_cast<float>(center.x());
	auto centerY = static_cast<float>(center.y());
	matrix.translate(centerX, centerY);
	buffer->transform.apply(matrix);
	matrix.translate(-centerX, -centerY);

	auto viewRect = matrix.mapRect(rect);
	auto bufferSize = buffer->size().toSizeF();

	bufferSize.scale(viewRect.width(), viewRect.height(), Qt::KeepAspectRatio);
	this->imageNode->setRect(
	    viewRect.x() + viewRect.width() / 2 - bufferSize.width() / 2,
	    viewRect.y() + viewRect.height() / 2 - bufferSize.height() / 2,
	    bufferSize.width(),
	    bufferSize.height()
	);

	this->setMatrix(matrix);
}

void WlBufferQSGDisplayNode::syncSwapchain(const WlBufferSwapchain& swapchain) {
	auto* buffer = swapchain.frontbuffer();
	auto& texture = swapchain.presentSecondBuffer ? this->buffer2 : this->buffer1;

	if (swapchain.presentSecondBuffer == this->presentSecondBuffer && texture.first == buffer) {
		return;
	}

	this->presentSecondBuffer = swapchain.presentSecondBuffer;

	if (texture.first == buffer) {
		texture.second->sync(texture.first, this->window);
	} else {
		texture.first = buffer;
		texture.second.reset(buffer->createQsgTexture(this->window));
	}

	this->imageNode->setTexture(texture.second->texture());
}

} // namespace qs::wayland::buffer
