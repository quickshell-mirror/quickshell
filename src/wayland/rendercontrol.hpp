#pragma once

#include <memory>
#include <vector>

#include <qcoreevent.h>
#include <qimage.h>
#include <qobject.h>
#include <qpoint.h>
#include <qquickwindow.h>
#include <qsize.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <rhi/qrhi.h>

#include "../window/rendercontrol.hpp"
#include "buffer/manager.hpp"
#include "buffer/qsg.hpp"

namespace qs::window {
class WindowDamageTracker;
}

namespace qs::wayland::render {

class WlBufferRef;

class SurfaceBuffer {
public:
	SurfaceBuffer() = default;
	~SurfaceBuffer();
	Q_DISABLE_COPY_MOVE(SurfaceBuffer);

	[[nodiscard]] bool busy() const;

	QSize size;
	bool alpha = false;
	std::unique_ptr<WlBufferRef> ref;
	// Keep the dmabuf alive until the GL objects are destroyed.
	std::unique_ptr<buffer::WlBuffer> buffer;
	std::unique_ptr<buffer::WlBufferQSGTexture> import;
	std::unique_ptr<QRhiTexture> texture;
	std::shared_ptr<QRhiRenderBuffer> depthStencil;
	std::unique_ptr<QRhiRenderPassDescriptor> renderPass;
	std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
};

class SurfaceBufferPool {
public:
	[[nodiscard]] bool available(QSize size, bool alpha) const;
	// Requires available(size, alpha).
	SurfaceBuffer* acquire(QQuickWindow* window, QRhi* rhi, QSize size, bool alpha);
	void clear();

private:
	SurfaceBuffer* create(QQuickWindow* window, QRhi* rhi, QSize size, bool alpha);

	std::vector<std::unique_ptr<SurfaceBuffer>> buffers;
	std::shared_ptr<QRhiRenderBuffer> depthStencil;
};

// Separate content target for drawDecoration().
struct DecorationTarget {
	QSize size;
	std::unique_ptr<QRhiTexture> content;
	std::unique_ptr<QRhiRenderBuffer> depthStencil;
	std::unique_ptr<QRhiRenderPassDescriptor> renderPass;
	std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
	std::unique_ptr<QRhiTexture> decoration;
	qint64 decorationKey = 0;
};

class WaylandRenderControl: public qs::window::WindowRenderControl {
	Q_OBJECT;

public:
	WaylandRenderControl();
	~WaylandRenderControl() override;
	Q_DISABLE_COPY_MOVE(WaylandRenderControl);

	void requestCommit() override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
	void scheduleFrame();
	void onRenderRequested();

private:
	bool initializeScene();
	void renderFrame();
	void abortFrame();
	void commitWithoutFrame();
	QRhiRenderTarget* decorationTarget(QSize size);
	bool drawDecoration(const QImage& image, SurfaceBuffer* buffer, QPoint offset);

	std::shared_ptr<QRhi> rhi;
	SurfaceBufferPool pool;
	std::unique_ptr<DecorationTarget> decoration;
	qs::window::WindowDamageTracker* tracker = nullptr;
	bool commitRequested = false;
	bool presented = false;
	bool repaintRequested = false;
	bool polishing = false;
};

} // namespace qs::wayland::render
