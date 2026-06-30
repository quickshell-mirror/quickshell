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
}

QList<QuickshellScreenInfo*> WindowsetProjection::screens() {
	QList<QuickshellScreenInfo*> screens;

	for (auto* screen: this->bQScreens.value()) {
		if (auto* qsScreen = QuickshellTracked::instance()->screenInfo(screen)) {
			screens.append(qsScreen);
		} else {
			qCDebug(logWorkspace) << this << "screens list contains invalid screen"
			                      << static_cast<void*>(screen)
			                      << "which was probably removed before workspace state has settled.";
		}
	}

	return screens;
}

} // namespace qs::wm
