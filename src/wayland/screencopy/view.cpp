#include "view.hpp"

#include <qobject.h>
#include <qqmlinfo.h>
#include <qquickitem.h>
#include <qsize.h>
#include <qtmetamacros.h>

#include "../buffer/manager.hpp"
#include "../buffer/qsg.hpp"
#include "manager.hpp"

namespace qs::wayland::screencopy {

void ScreencopyView::setCaptureSource(QObject* captureSource) {
	if (captureSource == this->mCaptureSource) return;
	auto hadContext = this->context != nullptr;
	this->destroyContext(false);

	this->mCaptureSource = captureSource;

	if (captureSource) {
		QObject::connect(
		    captureSource,
		    &QObject::destroyed,
		    this,
		    &ScreencopyView::onCaptureSourceDestroyed
		);

		if (this->completed) this->createContext();
	}

	if (!this->context && hadContext) this->update();
	emit this->captureSourceChanged();
}

void ScreencopyView::onCaptureSourceDestroyed() {
	this->mCaptureSource = nullptr;
	this->destroyContext();
}

void ScreencopyView::setPaintCursors(bool paintCursors) {
	if (paintCursors == this->mPaintCursors) return;
	this->mPaintCursors = paintCursors;
	if (this->completed && this->context) this->createContext();
	emit this->paintCursorsChanged();
}

void ScreencopyView::setLive(bool live) {
	if (live == this->mLive) return;

	if (live && !this->mLive && this->context) {
		this->context->captureFrame();
	}

	this->mLive = live;
	emit this->liveChanged();
}

void ScreencopyView::createContext() {
	this->destroyContext(false);
	this->context = ScreencopyManager::createContext(this->mCaptureSource, this->mPaintCursors);

	if (!this->context) {
		qmlWarning(this) << "Capture source set to non captureable object.";
		return;
	}

	QObject::connect(
	    this->context,
	    &ScreencopyContext::stopped,
	    this,
	    &ScreencopyView::destroyContextWithUpdate
	);

	QObject::connect(
	    this->context,
	    &ScreencopyContext::frameCaptured,
	    this,
	    &ScreencopyView::onFrameCaptured
	);

	this->context->captureFrame();
}

void ScreencopyView::destroyContext(bool update) {
	auto hadContext = this->context != nullptr;
	delete this->context;
	this->context = nullptr;
	this->bHasContent = false;
	this->bSourceSize = QSize();
	if (hadContext && update) this->update();
}

void ScreencopyView::captureFrame() {
	if (this->context) this->context->captureFrame();
	else qmlWarning(this) << "Cannot capture frame, as no recording context is ready.";
}

void ScreencopyView::onFrameCaptured() {
	this->setFlag(QQuickItem::ItemHasContents);
	this->update();
	this->bHasContent = true;
	this->bSourceSize = this->context->swapchain().frontbuffer()->size();
}

void ScreencopyView::componentComplete() {
	this->QQuickItem::componentComplete();

	auto* bufManager = buffer::WlBufferManager::instance();
	if (!bufManager->isReady()) {
		QObject::connect(
		    bufManager,
		    &buffer::WlBufferManager::ready,
		    this,
		    &ScreencopyView::onBuffersReady
		);
	} else {
		this->onBuffersReady();
	}
}

void ScreencopyView::onBuffersReady() {
	this->completed = true;
	if (this->mCaptureSource) this->createContext();
}

QSGNode* ScreencopyView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* /*unused*/) {
	if (!this->context || !this->bHasContent) {
		delete oldNode;
		this->setFlag(QQuickItem::ItemHasContents, false);
		return nullptr;
	}

	auto* node = static_cast<buffer::WlBufferQSGDisplayNode*>(oldNode); // NOLINT

	if (!node) {
		node = new buffer::WlBufferQSGDisplayNode(this->window());
	}

	auto& swapchain = this->context->swapchain();
	node->syncSwapchain(swapchain);
	node->setRect(this->boundingRect());

	if (this->mLive) this->context->captureFrame();
	return node;
}

} // namespace qs::wayland::screencopy
