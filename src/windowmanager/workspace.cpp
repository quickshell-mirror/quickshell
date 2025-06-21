#include "workspace.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtmetamacros.h>

#include "../core/qmlglobal.hpp"

namespace qs::wm {

Q_LOGGING_CATEGORY(logWorkspace, "quickshell.wm.workspace", QtWarningMsg);

void Workspace::activate() { qCCritical(logWorkspace) << this << "cannot be activated"; }
void Workspace::deactivate() { qCCritical(logWorkspace) << this << "cannot be deactivated"; }
void Workspace::remove() { qCCritical(logWorkspace) << this << "cannot be removed"; }

void Workspace::setGroup(WorkspaceGroup* /*group*/) {
	qCCritical(logWorkspace) << this << "cannot be assigned to a group";
}

void WorkspaceGroup::onScreensChanged() {
	mCachedScreens.clear();

	for (auto* screen: this->bScreens.value()) {
		mCachedScreens.append(QuickshellTracked::instance()->screenInfo(screen));
	}

	emit this->screensChanged();
}

} // namespace qs::wm
