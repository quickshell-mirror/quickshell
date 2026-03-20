#include "manager.hpp"
#include <cstdint>

#include <private/qwaylandwindow_p.h>
#include <qtmetamacros.h>
#include <qwayland-ext-background-effect-v1.h>
#include <qwaylandclientextension.h>

#include "surface.hpp"

namespace qs::wayland::background_effect::impl {

BackgroundEffectManager::BackgroundEffectManager(): QWaylandClientExtensionTemplate(1) {
	this->initialize();
}

BackgroundEffectSurface*
BackgroundEffectManager::createEffectSurface(QtWaylandClient::QWaylandWindow* window) {
	return new BackgroundEffectSurface(this->get_background_effect(window->surface()));
}

bool BackgroundEffectManager::blurAvailable() const {
	return this->isActive() && this->mBlurAvailable;
}

void BackgroundEffectManager::ext_background_effect_manager_v1_capabilities(uint32_t flags) {
	auto available = static_cast<bool>(flags & capability_blur);
	if (available == this->mBlurAvailable) return;
	this->mBlurAvailable = available;
	emit this->blurAvailableChanged();
}

BackgroundEffectManager* BackgroundEffectManager::instance() {
	static auto* instance = new BackgroundEffectManager(); // NOLINT
	return instance->isInitialized() ? instance : nullptr;
}

} // namespace qs::wayland::background_effect::impl
