#pragma once

#include <private/qwaylandwindow_p.h>
#include <qpair.h>
#include <qproperty.h>
#include <qtclasshelpermacros.h>
#include <qwayland-keyboard-shortcuts-inhibit-unstable-v1.h>
#include <qwaylandclientextension.h>

#include "../../core/logcat.hpp"
#include "wayland-keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h"

namespace qs::wayland::shortcuts_inhibit::impl {

QS_DECLARE_LOGGING_CATEGORY(logShortcutsInhibit);

class ShortcutsInhibitor;

class ShortcutsInhibitManager
    : public QWaylandClientExtensionTemplate<ShortcutsInhibitManager>
    , public QtWayland::zwp_keyboard_shortcuts_inhibit_manager_v1 {
public:
	explicit ShortcutsInhibitManager();

	ShortcutsInhibitor* createShortcutsInhibitor(QtWaylandClient::QWaylandWindow* surface);
	void unrefShortcutsInhibitor(ShortcutsInhibitor* inhibitor);

	static ShortcutsInhibitManager* instance();

private:
	QHash<wl_surface*, QPair<ShortcutsInhibitor*, int>> inhibitors;
};

class ShortcutsInhibitor
    : public QObject
    , public QtWayland::zwp_keyboard_shortcuts_inhibitor_v1 {
	Q_OBJECT;

public:
	explicit ShortcutsInhibitor(::zwp_keyboard_shortcuts_inhibitor_v1* inhibitor, wl_surface* surface)
	    : QtWayland::zwp_keyboard_shortcuts_inhibitor_v1(inhibitor)
	    , mSurface(surface)
	    , bActive(false) {}

	~ShortcutsInhibitor() override;
	Q_DISABLE_COPY_MOVE(ShortcutsInhibitor);

	[[nodiscard]] QBindable<bool> bindableActive() const { return &this->bActive; }
	[[nodiscard]] bool isActive() const { return this->bActive; }
	[[nodiscard]] wl_surface* surface() const { return this->mSurface; }

signals:
	void activeChanged();

protected:
	void zwp_keyboard_shortcuts_inhibitor_v1_active() override;
	void zwp_keyboard_shortcuts_inhibitor_v1_inactive() override;

private:
	wl_surface* mSurface;
	Q_OBJECT_BINDABLE_PROPERTY(ShortcutsInhibitor, bool, bActive, &ShortcutsInhibitor::activeChanged);
};

} // namespace qs::wayland::shortcuts_inhibit::impl