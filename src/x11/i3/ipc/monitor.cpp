#include "monitor.hpp"

#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "workspace.hpp"

namespace qs::i3::ipc {

I3Workspace* I3Monitor::focusedWorkspace() const { return this->mFocusedWorkspace; };
QVariantMap I3Monitor::lastIpcObject() const { return this->mLastIpcObject; };

void I3Monitor::updateFromObject(const QVariantMap& obj) {
	auto activeWorkspaceId = obj.value("current_workspace").value<QString>();
	auto rect = obj.value("rect").toMap();

	Qt::beginPropertyUpdateGroup();
	this->bId = obj.value("id").value<qint32>();
	this->bName = obj.value("name").value<QString>();
	this->bPower = obj.value("power").value<bool>();
	this->bX = rect.value("x").value<qint32>();
	this->bY = rect.value("y").value<qint32>();
	this->bWidth = rect.value("width").value<qint32>();
	this->bHeight = rect.value("height").value<qint32>();
	this->bScale = obj.value("scale").value<qreal>();
	this->bFocused = obj.value("focused").value<bool>();
	Qt::endPropertyUpdateGroup();

	if (activeWorkspaceId != this->mFocusedWorkspaceName) {
		auto* workspace = this->ipc->findWorkspaceByName(activeWorkspaceId);
		if (activeWorkspaceId.isEmpty() || workspace == nullptr) { // is null when output is disabled
			this->mFocusedWorkspace = nullptr;
			this->mFocusedWorkspaceName = "";
		} else {
			this->mFocusedWorkspaceName = activeWorkspaceId;
			this->mFocusedWorkspace = workspace;
		}
		emit this->focusedWorkspaceChanged();
	};

	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}
}

void I3Monitor::setFocusedWorkspace(I3Workspace* workspace) {
	this->mFocusedWorkspace = workspace;
	this->mFocusedWorkspaceName = workspace->bindableName().value();
	emit this->focusedWorkspaceChanged();
};

} // namespace qs::i3::ipc
