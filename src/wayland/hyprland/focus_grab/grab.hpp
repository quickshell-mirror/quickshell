#pragma once

#include <private/qwaylandwindow_p.h>
#include <qlist.h>
#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qwayland-hyprland-focus-grab-v1.h>
#include <qwindow.h>
#include <wayland-hyprland-focus-grab-v1-client-protocol.h>

namespace qs::hyprland::focus_grab {
using HyprlandFocusGrab = QtWayland::hyprland_focus_grab_v1;

class FocusGrab
    : public QObject
    , public HyprlandFocusGrab {
	using QWaylandWindow = QtWaylandClient::QWaylandWindow;

	Q_OBJECT;

public:
	explicit FocusGrab(::hyprland_focus_grab_v1* grab);
	~FocusGrab() override;
	Q_DISABLE_COPY_MOVE(FocusGrab);

	[[nodiscard]] bool isActive() const;
	void addWindow(QWindow* window);
	void removeWindow(QWindow* window);
	void sync();
	void startTransaction();
	void completeTransaction();

signals:
	void activated();
	void cleared();

private:
	void hyprland_focus_grab_v1_cleared() override;

	void addWaylandWindow(QWaylandWindow* window);

	QList<QWaylandWindow*> pendingAdditions;
	bool commitRequired = false;
	bool transactionActive = false;
	bool active = false;
};

} // namespace qs::hyprland::focus_grab
