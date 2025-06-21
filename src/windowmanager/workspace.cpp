#include "workspace.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>

namespace qs::wm {

Q_LOGGING_CATEGORY(logWorkspace, "qs.wm.workspace", QtWarningMsg);

void Workspace::activate() { qCCritical(logWorkspace) << this << "cannot be activated"; }

void Workspace::deactivate() { qCCritical(logWorkspace) << this << "cannot be deactivated"; }

void Workspace::remove() { qCCritical(logWorkspace) << this << "cannot be removed"; }

} // namespace qs::wm
