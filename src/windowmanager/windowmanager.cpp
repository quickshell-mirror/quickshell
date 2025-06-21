#include "windowmanager.hpp"
#include <functional>
#include <utility>

namespace qs::wm {

std::function<WindowManager*()> WindowManager::provider;

void WindowManager::setProvider(std::function<WindowManager*()> provider) {
	WindowManager::provider = std::move(provider);
}

WindowManager* WindowManager::instance() {
	static auto* instance = WindowManager::provider();
	return instance;
}

} // namespace qs::wm
