#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-hyprland-global-shortcuts-v1.h>

namespace qs::hyprland::global_shortcuts::impl {

class GlobalShortcut
    : public QObject
    , public QtWayland::hyprland_global_shortcut_v1 {
	Q_OBJECT;

public:
	explicit GlobalShortcut(::hyprland_global_shortcut_v1* shortcut);
	~GlobalShortcut() override;
	Q_DISABLE_COPY_MOVE(GlobalShortcut);

signals:
	void pressed();
	void released();

private:
	// clang-format off
	void hyprland_global_shortcut_v1_pressed(quint32 tvSecHi, quint32 tvSecLo, quint32 tvNsec) override;
	void hyprland_global_shortcut_v1_released(quint32 tvSecHi, quint32 tvSecLo, quint32 tvNsec) override;
	// clang-format on
};

} // namespace qs::hyprland::global_shortcuts::impl
