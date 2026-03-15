#pragma once

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwayland-ext-background-effect-v1.h>
#include <qwaylandclientextension.h>

#include "surface.hpp"

namespace qs::wayland::background_effect::impl {

class BackgroundEffectManager
    : public QWaylandClientExtensionTemplate<BackgroundEffectManager>
    , public QtWayland::ext_background_effect_manager_v1 {
	Q_OBJECT;

public:
	explicit BackgroundEffectManager();

	BackgroundEffectSurface* createEffectSurface(QtWaylandClient::QWaylandWindow* window);

	[[nodiscard]] bool blurAvailable() const;

	static BackgroundEffectManager* instance();

signals:
	void blurAvailableChanged();

protected:
	void ext_background_effect_manager_v1_capabilities(uint32_t flags) override;

private:
	bool mBlurAvailable = false;
};

} // namespace qs::wayland::background_effect::impl
