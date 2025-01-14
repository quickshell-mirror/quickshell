#pragma once

#include <memory>

#include <qcontainerfwd.h>
#include <qquickwindow.h>
#include <qsgimagenode.h>
#include <qsgnode.h>
#include <qsgtexture.h>
#include <qvectornd.h>

#include "manager.hpp"

namespace qs::wayland::buffer {

// Interact only from QSG thread.
class WlBufferQSGTexture {
public:
	virtual ~WlBufferQSGTexture() = default;
	Q_DISABLE_COPY_MOVE(WlBufferQSGTexture);

	[[nodiscard]] virtual QSGTexture* texture() const = 0;
	virtual void sync(const WlBuffer* /*buffer*/, QQuickWindow* /*window*/) {}

protected:
	WlBufferQSGTexture() = default;
};

// Interact only from QSG thread.
class WlBufferQSGDisplayNode: public QSGTransformNode {
public:
	explicit WlBufferQSGDisplayNode(QQuickWindow* window);

	void syncSwapchain(const WlBufferSwapchain& swapchain);
	void setRect(const QRectF& rect);

private:
	QQuickWindow* window;
	QSGImageNode* imageNode;
	QPair<WlBuffer*, std::unique_ptr<WlBufferQSGTexture>> buffer1;
	QPair<WlBuffer*, std::unique_ptr<WlBufferQSGTexture>> buffer2;
	bool presentSecondBuffer = false;
};

} // namespace qs::wayland::buffer
