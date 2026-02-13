#pragma once

#include <qregion.h>
#include <qtclasshelpermacros.h>
#include <qwayland-ext-background-effect-v1.h>

namespace qs::wayland::background_effect::impl {

class BackgroundEffectSurface: public QtWayland::ext_background_effect_surface_v1 {
public:
	explicit BackgroundEffectSurface(::ext_background_effect_surface_v1* surface);
	~BackgroundEffectSurface() override;
	Q_DISABLE_COPY_MOVE(BackgroundEffectSurface);

	void setBlurRegion(const QRegion& region);
	void setInert();

private:
	bool mInert = false;
};

} // namespace qs::wayland::background_effect::impl
