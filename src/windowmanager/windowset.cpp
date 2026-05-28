#include "windowset.hpp"

#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>

#include "../core/qmlglobal.hpp"
#include "windowmanager.hpp"

namespace qs::wm {

Q_LOGGING_CATEGORY(logWorkspace, "quickshell.wm.workspace", QtWarningMsg);

void Windowset::activate() { qCCritical(logWorkspace) << this << "cannot be activated"; }
void Windowset::deactivate() { qCCritical(logWorkspace) << this << "cannot be deactivated"; }
void Windowset::remove() { qCCritical(logWorkspace) << this << "cannot be removed"; }

void Windowset::setProjection(WindowsetProjection* /*projection*/) {
	qCCritical(logWorkspace) << this << "cannot be assigned to a projection";
}

WindowsetProjection::WindowsetProjection(QObject* parent): QObject(parent) {
	this->bWindowsets.setBinding([this] {
		QList<Windowset*> result;
		for (auto* ws: WindowManager::instance()->bindableWindowsets().value()) {
			if (ws->bindableProjection().value() == this) {
				result.append(ws);
			}
		}
		return result;
	});

	this->bScreens.setBinding([this] {
		QList<QuickshellScreenInfo*> screens;

		for (auto* screen: this->bQScreens.value()) {
			screens.append(QuickshellTracked::instance()->screenInfo(screen));
		}

		return screens;
	});
}

} // namespace qs::wm
