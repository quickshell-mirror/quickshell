#include "screenprojection.hpp"

#include <qlist.h>
#include <qobject.h>
#include <qscreen.h>

#include "windowmanager.hpp"
#include "windowset.hpp"

namespace qs::wm {

ScreenProjection::ScreenProjection(QScreen* screen, QObject* parent)
    : WindowsetProjection(parent)
    , mScreen(screen) {
	this->bQScreens = {screen};
	this->bWindowsets.setBinding([this]() {
		QList<Windowset*> result;
		for (auto* ws: WindowManager::instance()->bindableWindowsets().value()) {
			auto* proj = ws->bindableProjection().value();
			if (proj && proj->bindableQScreens().value().contains(this->mScreen)) {
				result.append(ws);
			}
		}
		return result;
	});
}

QScreen* ScreenProjection::screen() const { return this->mScreen; }

} // namespace qs::wm
