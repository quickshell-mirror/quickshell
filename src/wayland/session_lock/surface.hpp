#pragma once

#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandshmbackingstore_p.h>
#include <private/qwaylandwindow_p.h>
#include <qregion.h>
#include <qtclasshelpermacros.h>
#include <qtversionchecks.h>
#include <qtypes.h>
#include <qwayland-ext-session-lock-v1.h>

#include "session_lock.hpp"

class QSWaylandSessionLockSurface
    : public QtWaylandClient::QWaylandShellSurface
    , public QtWayland::ext_session_lock_surface_v1 {
public:
	QSWaylandSessionLockSurface(QtWaylandClient::QWaylandWindow* window);
	~QSWaylandSessionLockSurface() override;
	Q_DISABLE_COPY_MOVE(QSWaylandSessionLockSurface);

	[[nodiscard]] bool isExposed() const override;
	void applyConfigure() override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
	[[nodiscard]] bool commitSurfaceRole() const override;
#else
	bool handleExpose(const QRegion& region) override;
#endif

	void setExtension(LockWindowExtension* ext);
	void setVisible();

private:
	void
	ext_session_lock_surface_v1_configure(quint32 serial, quint32 width, quint32 height) override;

#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
	void initVisible();
	bool visible = false;
	QtWaylandClient::QWaylandShmBuffer* initBuf = nullptr;
#endif

	LockWindowExtension* ext = nullptr;
	QSize size;
	bool configured = false;
};
