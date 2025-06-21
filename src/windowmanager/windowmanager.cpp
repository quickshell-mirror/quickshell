#include "windowmanager.hpp"
#include <functional>
#include <utility>

#include <qobject.h>

#include "../core/qmlscreen.hpp"
#include "screenprojection.hpp"

namespace qs::wm {

std::function<WindowManager*()> WindowManager::provider;

void WindowManager::setProvider(std::function<WindowManager*()> provider) {
	WindowManager::provider = std::move(provider);
}

WindowManager* WindowManager::instance() {
	static auto* instance = WindowManager::provider();
	return instance;
}

ScreenProjection* WindowManager::screenProjection(QuickshellScreenInfo* screen) {
	auto* qscreen = screen->screen;
	auto it = this->mScreenProjections.find(qscreen);
	if (it != this->mScreenProjections.end()) {
		return *it;
	}

	auto* projection = new ScreenProjection(qscreen, this);
	this->mScreenProjections.insert(qscreen, projection);

	QObject::connect(qscreen, &QObject::destroyed, this, [this, projection, qscreen]() {
		this->mScreenProjections.remove(qscreen);
		delete projection;
	});

	return projection;
}

} // namespace qs::wm
