#include "surface.hpp"

#include <private/qwaylandscreen_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtversionchecks.h>
#include <qtypes.h>

#include "lock.hpp"
#include "session_lock.hpp"

QSWaylandSessionLockSurface::QSWaylandSessionLockSurface(QtWaylandClient::QWaylandWindow* window)
    : QtWaylandClient::QWaylandShellSurface(window) {
	auto* qwindow = window->window();
	this->setExtension(LockWindowExtension::get(qwindow));

	if (this->ext == nullptr) {
		qFatal() << "QSWaylandSessionLockSurface created with null LockWindowExtension";
	}

	if (this->ext->lock == nullptr) {
		qFatal() << "QSWaylandSessionLock for QSWaylandSessionLockSurface died";
	}

	wl_output* output = nullptr; // NOLINT (include)
	auto* waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(qwindow->screen()->handle());

	if (waylandScreen != nullptr && !waylandScreen->isPlaceholder() && waylandScreen->output()) {
		output = waylandScreen->output();
	} else {
		qFatal() << "Session lock screen does not corrospond to a real screen. Force closing window";
	}

	this->init(this->ext->lock->get_lock_surface(window->waylandSurface()->object(), output));
}

QSWaylandSessionLockSurface::~QSWaylandSessionLockSurface() {
	if (this->ext != nullptr) this->ext->surface = nullptr;
	this->destroy();
}

bool QSWaylandSessionLockSurface::isExposed() const { return this->configured; }

void QSWaylandSessionLockSurface::applyConfigure() {
	this->window()->resizeFromApplyConfigure(this->size);
}

void QSWaylandSessionLockSurface::setExtension(LockWindowExtension* ext) {
	if (ext == nullptr) {
		if (this->window() != nullptr) this->window()->window()->close();
	} else {
		if (this->ext != nullptr) {
			this->ext->surface = nullptr;
		}

		this->ext = ext;
		this->ext->surface = this;
	}
}

void QSWaylandSessionLockSurface::ext_session_lock_surface_v1_configure(
    quint32 serial,
    quint32 width,
    quint32 height
) {
	if (!this->window()) return;

	this->ack_configure(serial);

	this->size = QSize(static_cast<qint32>(width), static_cast<qint32>(height));
	if (!this->configured) {
		this->configured = true;

		this->window()->resizeFromApplyConfigure(this->size);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
		this->window()->handleExpose(QRect(QPoint(), this->size));
#elif QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
		this->window()->sendRecursiveExposeEvent();
#else
		this->window()->updateExposure();
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
		if (this->visible) this->initVisible();
#endif
	} else {
		// applyConfigureWhenPossible runs too late and causes a protocol error on reconfigure.
		this->window()->resizeFromApplyConfigure(this->size);
	}
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)

bool QSWaylandSessionLockSurface::commitSurfaceRole() const { return false; }

void QSWaylandSessionLockSurface::setVisible() { this->window()->window()->setVisible(true); }

#else

bool QSWaylandSessionLockSurface::handleExpose(const QRegion& region) {
	if (this->initBuf != nullptr) {
		// at this point qt's next commit to the surface will have a new buffer, and we can safely delete this one.
		delete this->initBuf;
		this->initBuf = nullptr;
	}

	return this->QtWaylandClient::QWaylandShellSurface::handleExpose(region);
}

void QSWaylandSessionLockSurface::setVisible() {
	if (this->configured && !this->visible) this->initVisible();
	this->visible = true;
}

#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)

#include <private/qwaylandshmbackingstore_p.h>

void QSWaylandSessionLockSurface::initVisible() {
	this->visible = true;

	// qt always commits a null buffer in QWaylandWindow::initWindow.
	// We attach a dummy buffer to satisfy ext_session_lock_v1.
	this->initBuf = new QtWaylandClient::QWaylandShmBuffer(
	    this->window()->display(),
	    this->size,
	    QImage::Format_ARGB32
	);

	this->window()->waylandSurface()->attach(this->initBuf->buffer(), 0, 0);
	this->window()->window()->setVisible(true);
}

#elif QT_VERSION < QT_VERSION_CHECK(6, 10, 0)

#include <cmath>

#include <private/qwayland-wayland.h>
#include <private/qwaylanddisplay_p.h>
#include <qscopedpointer.h>
#include <qtclasshelpermacros.h>
#include <qtdeprecationdefinitions.h>

// As of Qt 6.9, a null buffer is unconditionally comitted to the surface. To avoid this, we
// cast the window to a subclass with access to mSurface, then swap mSurface during
// QWaylandWindow::initWindow to avoid the null commit.

// Since QWaylandWindow::mSurface is protected and not private, we can just pretend our
// QWaylandWindow is a subclass that can access it.
class SurfaceAccessor: public QtWaylandClient::QWaylandWindow {
public:
	QScopedPointer<QtWaylandClient::QWaylandSurface>& surfacePointer() { return this->mSurface; }
};

// QWaylandSurface is not exported, meaning we can't link to its constructor or destructor.
// As such, the best alternative is to create a class that can take its place during
// QWaylandWindow::init.
//
// Fortunately, QWaylandSurface's superclasses are both exported, and are therefore identical
// for the purpose of accepting calls during initWindow.
class HackSurface
    : public QObject
    , public QtWayland::wl_surface {
public:
	HackSurface(QtWaylandClient::QWaylandDisplay* display)
	    : wl_surface(display->createSurface(this)) {}
	~HackSurface() override { this->destroy(); }
	Q_DISABLE_COPY_MOVE(HackSurface);
};

void QSWaylandSessionLockSurface::initVisible() {
	this->visible = true;

	// See note above HackSurface.
	auto* dummySurface = new HackSurface(this->window()->display());
	auto* tempSurface =
	    new QScopedPointer(reinterpret_cast<QtWaylandClient::QWaylandSurface*>(dummySurface));

	auto& surfacePointer = reinterpret_cast<SurfaceAccessor*>(this->window())->surfacePointer();

	// Swap out the surface for a dummy during initWindow.
	QT_WARNING_PUSH
	QT_WARNING_DISABLE_DEPRECATED // swap()
	{
		surfacePointer.swap(*tempSurface);
		this->window()->window()->setVisible(true);
		surfacePointer.swap(*tempSurface);
	}
	QT_WARNING_POP

	// Cast to a HackSurface pointer so ~HackSurface is called instead of ~QWaylandSurface.
	delete reinterpret_cast<QScopedPointer<HackSurface>*>(tempSurface);

	// This would normally be run in QWaylandWindow::initWindow but isn't because we
	// fed that a dummy surface.
	if (surfacePointer->version() >= 3) {
		surfacePointer->set_buffer_scale(std::ceil(this->window()->scale()));
	}
}

#endif
