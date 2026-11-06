#include "rendercontrol.hpp"
#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include <libdrm/drm_fourcc.h>
#include <private/qquickwindow_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgrenderloop_p.h>
#include <private/qwaylandabstractdecoration_p.h>
#include <private/qwaylandbuffer_p.h>
#include <private/qwaylandwindow_p.h>
#include <qabstractanimation.h>
#include <qchronotimer.h>
#include <qcoreevent.h>
#include <qguiapplication.h>
#include <qimage.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmargins.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qoffscreensurface.h> // NOLINT(misc-include-cleaner)
#include <qpa/qplatformwindow.h>
#include <qpoint.h>
#include <qquickgraphicsdevice.h>
#include <qquickrendertarget.h>
#include <qquickwindow.h>
#include <qrect.h>
#include <qregion.h>
#include <qscreen.h>
#include <qsize.h>
#include <qstring.h>
#include <qsurfaceformat.h>
#include <qtclasshelpermacros.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <wayland-client-protocol.h>

#include "../core/logcat.hpp"
#include "../window/damage.hpp"
#include "../window/proxywindow.hpp"
#include "../window/rendercontrol.hpp"
#include "buffer/manager.hpp"

namespace qs::wayland::render {

namespace {

QS_LOGGING_CATEGORY(logRenderControl, "quickshell.wayland.rendercontrol", QtWarningMsg);

constexpr qsizetype MAX_BUFFERS = 3;

// Keep the context offscreen; leave the window out of the init params.
std::shared_ptr<QRhi> sharedRhi(const QSurfaceFormat& format) {
	static std::weak_ptr<QRhi> shared;
	if (auto rhi = shared.lock()) return rhi;

	auto* surface = QRhiGles2InitParams::newFallbackSurface(format);
	QRhiGles2InitParams params;
	params.format = format;
	params.fallbackSurface = surface;

	auto* rhi = QRhi::create(QRhi::OpenGLES2, &params);
	if (!rhi) {
		delete surface;
		return nullptr;
	}

	auto result = std::shared_ptr<QRhi>(rhi, [surface](QRhi* rhi) {
		delete rhi;
		delete surface;
	});

	shared = result;
	return result;
}

class AnimationTimer: public QChronoTimer {
public:
	explicit AnimationTimer(QAnimationDriver* driver): QChronoTimer(driver) {
		this->setTimerType(Qt::PreciseTimer);
		QObject::connect(this, &QChronoTimer::timeout, driver, &QAnimationDriver::advance);

		auto* app = static_cast<QGuiApplication*>(QGuiApplication::instance()); // NOLINT
		QObject::connect(app, &QGuiApplication::screenAdded, this, &AnimationTimer::onScreenAdded);
		QObject::connect(app, &QGuiApplication::screenRemoved, this, &AnimationTimer::updateInterval);
		for (auto* screen: QGuiApplication::screens()) this->onScreenAdded(screen);
	}

private:
	void onScreenAdded(QScreen* screen) {
		QObject::connect(screen, &QScreen::refreshRateChanged, this, &AnimationTimer::updateInterval);
		this->updateInterval();
	}

	// The driver advances by wall time, so slower screens skip ticks instead of lagging.
	void updateInterval() {
		auto refreshRate = 60.0;
		for (auto* screen: QGuiApplication::screens()) {
			refreshRate = std::max(refreshRate, screen->refreshRate());
		}

		auto interval = std::chrono::duration<double>(1 / refreshRate);
		this->setInterval(std::chrono::round<std::chrono::nanoseconds>(interval));
	}
};

// Drive animations without Qt-owned windows; see QSGThreadedRenderLoop::polishAndSync().
void driveAnimations() {
	static auto installed = false;
	if (installed) return;
	installed = true;

	auto* loop = QSGRenderLoop::instance();
	auto* driver = loop->animationDriver();
	if (!driver || loop->sceneGraphContext()->isVSyncDependent(driver)) return;

	auto* timer = new AnimationTimer(driver);
	QObject::connect(driver, &QAnimationDriver::started, timer, &QChronoTimer::start);
	QObject::connect(driver, &QAnimationDriver::stopped, timer, &QChronoTimer::stop);
	if (driver->isRunning()) timer->start();
}

// Borrow the wl_buffer for one commit; SurfaceBufferPool owns it and handles releases.
class WlBufferRef: public QtWaylandClient::QWaylandBuffer {
public:
	WlBufferRef(wl_buffer* buffer, QSize size): mSize(size) { this->mBuffer = buffer; }
	~WlBufferRef() override { this->mBuffer = nullptr; }
	Q_DISABLE_COPY_MOVE(WlBufferRef);

	[[nodiscard]] QSize size() const override { return this->mSize; }

private:
	QSize mSize;
};

qs::window::WindowRenderControl* createRenderControl() {
	if (qEnvironmentVariableIsSet("QS_DISABLE_DMABUF")) {
		qCInfo(logRenderControl) << "Render control is unavailable with QS_DISABLE_DMABUF.";
		return nullptr;
	}

	return new WaylandRenderControl();
}

} // namespace

SurfaceBuffer::~SurfaceBuffer() = default;

const wl_buffer_listener SurfaceBufferPool::LISTENER = {.release = &SurfaceBufferPool::onRelease};

void SurfaceBufferPool::onRelease(void* data, wl_buffer* /*buffer*/) {
	auto* buffer = static_cast<SurfaceBuffer*>(data);
	buffer->busy = false;
	buffer->pool->control->onBufferReleased();
}

SurfaceBuffer* SurfaceBufferPool::acquire(QQuickWindow* window, QRhi* rhi, QSize size, bool alpha) {
	auto matches = [&](const std::unique_ptr<SurfaceBuffer>& buffer) {
		return buffer->size == size && buffer->alpha == alpha;
	};

	// Keep old buffers until onRelease() clears busy.
	std::erase_if(this->buffers, [&](const auto& buffer) {
		return !matches(buffer) && !buffer->busy;
	});

	for (const auto& buffer: this->buffers) {
		if (!matches(buffer) || buffer->busy) continue;
		this->current = buffer.get();
		return this->current;
	}

	this->current = this->create(window, rhi, size, alpha);
	return this->current;
}

void SurfaceBufferPool::clear() {
	this->current = nullptr;
	this->buffers.clear();
	this->depthStencil.reset();
}

bool SurfaceBufferPool::trim() {
	auto idle = [this](const auto& buffer) { return buffer.get() != this->current && !buffer->busy; };
	return std::erase_if(this->buffers, idle) != 0;
}

bool SurfaceBufferPool::available(QSize size, bool alpha) const {
	auto used = 0;
	for (const auto& buffer: this->buffers) {
		if (buffer->size != size || buffer->alpha != alpha) continue;
		if (!buffer->busy) return true;
		used++;
	}

	return used < MAX_BUFFERS;
}

SurfaceBuffer* SurfaceBufferPool::create(QQuickWindow* window, QRhi* rhi, QSize size, bool alpha) {
	buffer::WlBufferRequest request;
	request.width = size.width();
	request.height = size.height();
	request.dmabuf.formats.push(
	    buffer::WlBufferRequest::DmaFormat(alpha ? DRM_FORMAT_ARGB8888 : DRM_FORMAT_XRGB8888)
	);

	auto result = std::make_unique<SurfaceBuffer>();
	result->pool = this;
	result->size = size;
	result->alpha = alpha;
	result->buffer.reset(buffer::WlBufferManager::instance()->createBuffer(request));
	if (!result->buffer) return nullptr;
	wl_buffer_add_listener(result->buffer->buffer(), &LISTENER, result.get());

	result->import.reset(result->buffer->createQsgTexture(window));
	if (!result->import) return nullptr;

	auto native = result->import->texture()->rhiTexture()->nativeTexture();
	result->texture.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget));
	if (!result->texture->createFrom({.object = native.object, .layout = 0})) return nullptr;

	if (!this->depthStencil || this->depthStencil->pixelSize() != size) {
		this->depthStencil.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size));
		if (!this->depthStencil->create()) {
			this->depthStencil.reset();
			return nullptr;
		}
	}

	result->depthStencil = this->depthStencil;

	auto description = QRhiTextureRenderTargetDescription(QRhiColorAttachment(result->texture.get()));
	description.setDepthStencilBuffer(result->depthStencil.get());
	result->renderTarget.reset(rhi->newTextureRenderTarget(description));
	result->renderPass.reset(result->renderTarget->newCompatibleRenderPassDescriptor());
	result->renderTarget->setRenderPassDescriptor(result->renderPass.get());
	if (!result->renderTarget->create()) return nullptr;

	return this->buffers.emplace_back(std::move(result)).get();
}

WaylandRenderControl::WaylandRenderControl(): pool(this) {
	this->trimTimer.setSingleShot(true);
	this->trimTimer.setInterval(std::chrono::seconds(1));

	// clang-format off
	QObject::connect(this, &QQuickRenderControl::renderRequested, this, &WaylandRenderControl::onRenderRequested);
	QObject::connect(this, &QQuickRenderControl::sceneChanged, this, &WaylandRenderControl::scheduleFrame);
	QObject::connect(&this->trimTimer, &QTimer::timeout, this, &WaylandRenderControl::trimBuffers);
	// clang-format on
}

WaylandRenderControl::~WaylandRenderControl() {
	if (!this->rhi) return;

	// Keep the context current through texture and scene graph cleanup.
	this->rhi->makeThreadLocalNativeContextCurrent();
	this->pool.clear();
	this->decoration.reset();
	// Invalidate before releasing rhi; QQuickRenderControl::~QQuickRenderControl() is too late.
	this->invalidate();
}

void WaylandRenderControl::requestCommit() {
	this->commitRequested = true;
	this->scheduleFrame();
}

// renderFrame() syncs polish changes in the current frame.
void WaylandRenderControl::scheduleFrame() {
	if (!this->polishing) this->window()->requestUpdate();
}

// Explicit repaint requests bypass the damage check in renderFrame().
void WaylandRenderControl::onRenderRequested() {
	this->repaintRequested = true;
	this->scheduleFrame();
}

void WaylandRenderControl::onBufferReleased() {
	this->trimTimer.start();
	if (std::exchange(this->bufferAwaited, false)) this->scheduleFrame();
}

void WaylandRenderControl::trimBuffers() {
	// WlDmaBufferQSGTexture's destructor needs the current context.
	if (!this->rhi->makeThreadLocalNativeContextCurrent()) return;
	if (!this->pool.trim()) return;

	// QRhiGles2 frees released GL objects in beginOffscreenFrame(), see executeDeferredReleases().
	QRhiCommandBuffer* commands = nullptr;
	if (this->rhi->beginOffscreenFrame(&commands) != QRhi::FrameOpSuccess) return;
	this->rhi->endOffscreenFrame();
}

bool WaylandRenderControl::eventFilter(QObject* watched, QEvent* event) {
	if (watched != this->window()) return false;

	switch (event->type()) {
	case QEvent::UpdateRequest: this->renderFrame(); break;
	case QEvent::Expose:
		if (!this->window()->isExposed()) {
			this->presented = false;
			break;
		}
		// Commit surface state even if renderFrame() finds no damage.
		this->commitRequested = true;
		// Deliver any update left pending while the window was unexposed.
		if (this->window()->handle()->hasPendingUpdateRequest()) {
			this->window()->handle()->deliverUpdateRequest();
			break;
		}
		this->renderFrame();
		break;
	default: break;
	}

	return false;
}

bool WaylandRenderControl::initializeScene() {
	auto* window = this->window();
	driveAnimations();

	// Connect the tracker before QQuickRenderControl::initialize() emits sceneGraphInitialized.
	auto* base = qobject_cast<QsQuickWindowBase*>(window);
	if (!this->tracker && base) {
		this->tracker = new qs::window::WindowDamageTracker(base);
	}

	this->rhi = sharedRhi(window->requestedFormat());
	if (this->rhi) window->setGraphicsDevice(QQuickGraphicsDevice::fromRhi(this->rhi.get()));

	if (!this->rhi || !this->initialize()) {
		this->rhi.reset();
		emit window->sceneGraphError(
		    QQuickWindow::ContextNotAvailable,
		    QStringLiteral("Failed to initialize the render control.")
		);
		return false;
	}

	auto* manager = buffer::WlBufferManager::instance();
	if (!manager->isReady()) {
		// clang-format off
		QObject::connect(manager, &buffer::WlBufferManager::ready, this, &WaylandRenderControl::scheduleFrame, Qt::SingleShotConnection);
		// clang-format on
		manager->initWindow(window);
	}

	return true;
}

void WaylandRenderControl::renderFrame() {
	auto* window = this->window();
	if (!window->isExposed()) return;
	// Allow the first frame with updates disabled; see QSGThreadedRenderLoop::handleExposure().
	if (this->presented && !QQuickWindowPrivate::get(window)->updatesEnabled) return;
	if (!this->rhi && !this->initializeScene()) return;
	if (!buffer::WlBufferManager::instance()->isReady()) return;

	this->polishing = true;
	this->polishItems();
	this->polishing = false;

	auto* waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
	if (!waylandWindow || !window->isExposed()) return;

	auto* decoration = waylandWindow->decoration();
	auto scale = waylandWindow->scale();
	auto surfaceSize = waylandWindow->surfaceSize();
	auto pixelSize = surfaceSize * scale;
	auto alpha = decoration || window->requestedFormat().hasAlpha();

	if (pixelSize.isEmpty()) {
		this->commitWithoutFrame();
		return;
	}

	// onBufferReleased() schedules another frame.
	if (!this->pool.available(pixelSize, alpha)) {
		this->bufferAwaited = true;
		return;
	}

	this->beginFrame();
	if (!this->rhi->isRecordingFrame()) {
		qCWarning(logRenderControl) << "Failed to begin a frame for" << window;
		this->commitWithoutFrame();
		return;
	}

	this->sync();

	// Emit frameSwapped even when damage tracking skips the render.
	auto* tracker = this->tracker;
	auto repaint = std::exchange(this->repaintRequested, false);
	if (tracker && !repaint && tracker->damage() == QRegion() && !tracker->pendingPreprocess()) {
		this->endFrame();
		this->commitWithoutFrame();
		QQuickWindowPrivate::get(window)->fireFrameSwapped();
		return;
	}

	if (!decoration) this->decoration.reset();

	// Acquire a buffer only after the damage check.
	QRhiRenderTarget* target = nullptr;
	auto* buffer = this->pool.acquire(window, this->rhi.get(), pixelSize, alpha);
	if (buffer) {
		target =
		    decoration ? this->decorationTarget(window->size() * scale) : buffer->renderTarget.get();
	}

	if (!target) {
		this->abortFrame();
		return;
	}

	auto renderTarget = QQuickRenderTarget::fromRhiRenderTarget(target);
	// Match the surface orientation when the framebuffer's Y axis points up.
	renderTarget.setMirrorVertically(this->rhi->isYUpInFramebuffer());
	window->setRenderTarget(renderTarget);

	if (tracker) tracker->beginRender();
	this->render();

	if (decoration) {
		auto margins = waylandWindow->clientSideMargins();
		auto offset = (QPointF(margins.left(), margins.top()) * scale).toPoint();
		if (!this->drawDecoration(decoration->contentImage(), buffer, offset)) {
			this->abortFrame();
			return;
		}
	}

	this->endFrame();

	// Collect changes made during render; WindowDamageTracker::sync() forces full damage with margins.
	auto damage = tracker ? tracker->damage() : std::nullopt;
	auto ref = WlBufferRef(buffer->buffer->buffer(), surfaceSize);
	waylandWindow->safeCommit(&ref, damage.value_or(QRect(QPoint(), surfaceSize)));
	QQuickWindowPrivate::get(window)->fireFrameSwapped();
	// Keep the damage until safeCommit() accepts a buffer.
	if (!ref.busy()) return;

	buffer->busy = true;
	this->commitRequested = false;
	this->presented = true;
	if (!tracker) return;
	tracker->presented();
	// Redraw stale layer textures carried over by SceneDamageTracker::presented().
	if (tracker->pendingDamage()) this->scheduleFrame();
}

void WaylandRenderControl::abortFrame() {
	this->endFrame();
	this->commitWithoutFrame();
	emit this->window()->sceneGraphError(
	    QQuickWindow::ContextNotAvailable,
	    QStringLiteral("Failed to create a buffer for the window.")
	);
}

void WaylandRenderControl::commitWithoutFrame() {
	if (!this->commitRequested) return;
	this->commitRequested = false;

	if (auto* window = dynamic_cast<QtWaylandClient::QWaylandWindow*>(this->window()->handle())) {
		window->commit();
	}
}

QRhiRenderTarget* WaylandRenderControl::decorationTarget(QSize size) {
	if (this->decoration && this->decoration->size == size) {
		return this->decoration->renderTarget.get();
	}

	auto target = std::make_unique<DecorationTarget>();
	target->size = size;
	target->content.reset(
	    this->rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget)
	);
	target->depthStencil.reset(this->rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size));

	auto description = QRhiTextureRenderTargetDescription(QRhiColorAttachment(target->content.get()));
	description.setDepthStencilBuffer(target->depthStencil.get());
	target->renderTarget.reset(this->rhi->newTextureRenderTarget(description));
	target->renderPass.reset(target->renderTarget->newCompatibleRenderPassDescriptor());
	target->renderTarget->setRenderPassDescriptor(target->renderPass.get());

	if (!target->content->create() || !target->depthStencil->create()
	    || !target->renderTarget->create())
	{
		return nullptr;
	}

	this->decoration = std::move(target);
	return this->decoration->renderTarget.get();
}

// Replace the decoration's content area, including transparent pixels.
bool WaylandRenderControl::drawDecoration(
    const QImage& image,
    SurfaceBuffer* buffer,
    QPoint offset
) {
	auto* target = this->decoration.get();

	if (!target->decoration || target->decoration->pixelSize() != image.size()) {
		target->decoration.reset(this->rhi->newTexture(QRhiTexture::RGBA8, image.size()));
		target->decorationKey = 0;
		if (!target->decoration->create()) {
			target->decoration.reset();
			return false;
		}
	}

	auto* updates = this->rhi->nextResourceUpdateBatch();
	if (image.cacheKey() != target->decorationKey) {
		target->decorationKey = image.cacheKey();
		auto converted = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
		updates->uploadTexture(target->decoration.get(), converted);
	}

	auto bufferRect = QRect(QPoint(), buffer->size);

	QRhiTextureCopyDescription frame;
	frame.setPixelSize(QRect(QPoint(), image.size()).intersected(bufferRect).size());
	updates->copyTexture(buffer->texture.get(), target->decoration.get(), frame);

	QRhiTextureCopyDescription content;
	content.setDestinationTopLeft(offset);
	content.setPixelSize(QRect(offset, target->size).intersected(bufferRect).size());
	updates->copyTexture(buffer->texture.get(), target->content.get(), content);

	this->commandBuffer()->resourceUpdate(updates);
	return true;
}

} // namespace qs::wayland::render

// NOLINTNEXTLINE(misc-use-internal-linkage)
void installRenderControl() {
	qs::window::WindowRenderControl::setFactory(&qs::wayland::render::createRenderControl);
}
