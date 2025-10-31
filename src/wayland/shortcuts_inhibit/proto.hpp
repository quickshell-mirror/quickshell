#pragma once

#include <private/qwaylandwindow_p.h>
#include <qtclasshelpermacros.h>
#include <qwayland-keyboard-shortcuts-inhibit-unstable-v1.h>
#include <qwaylandclientextension.h>

#include "wayland-keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h"

namespace qs::wayland::shortcuts_inhibit::impl {

class ShortcutsInhibitor;

class ShortcutsInhibitManager
    : public QWaylandClientExtensionTemplate<ShortcutsInhibitManager>
    , public QtWayland::zwp_keyboard_shortcuts_inhibit_manager_v1 {
public:
	explicit ShortcutsInhibitManager();

	ShortcutsInhibitor* createShortcutsInhibitor(QtWaylandClient::QWaylandWindow* surface);

	static ShortcutsInhibitManager* instance();
};

class ShortcutsInhibitor: public QtWayland::zwp_keyboard_shortcuts_inhibitor_v1 {
public:
	explicit ShortcutsInhibitor(::zwp_keyboard_shortcuts_inhibitor_v1* inhibitor)
	    : QtWayland::zwp_keyboard_shortcuts_inhibitor_v1(inhibitor) {}

	~ShortcutsInhibitor() override;
	Q_DISABLE_COPY_MOVE(ShortcutsInhibitor);
};

} // namespace qs::wayland::shortcuts_inhibit::impl