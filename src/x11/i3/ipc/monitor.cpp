#include "monitor.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "controller.hpp"
#include "workspace.hpp"

namespace qs::i3::ipc {

I3Monitor::I3Monitor(I3IpcController* ipc): QObject(ipc), ipc(ipc) {
	// clang-format off
	this->bFocused.setBinding([this]() { return this->ipc->bindableFocusedMonitor().value() == this; });
	// clang-format on
}

QVariantMap I3Monitor::lastIpcObject() const { return this->mLastIpcObject; };

void I3Monitor::updateFromObject(const QVariantMap& obj) {
	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}

	auto activeWorkspaceName = obj.value("current_workspace").value<QString>();
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

	if (!this->bActiveWorkspace
	    || activeWorkspaceName != this->bActiveWorkspace->bindableName().value())
	{
		if (activeWorkspaceName.isEmpty()) {
			this->setActiveWorkspace(nullptr);
		} else {
			this->setActiveWorkspace(this->ipc->findWorkspaceByName(activeWorkspaceName));
		}
	};

	Qt::endPropertyUpdateGroup();
}

void I3Monitor::updateInitial(const QString& name) { this->bName = name; }

void I3Monitor::setFocusedWorkspace(I3Workspace* workspace) { this->setActiveWorkspace(workspace); };

void I3Monitor::setActiveWorkspace(I3Workspace* workspace) {
	auto* oldWorkspace = this->bActiveWorkspace.value();

	if (oldWorkspace == workspace) return;

	if (oldWorkspace != nullptr) {
		QObject::disconnect(oldWorkspace, nullptr, this, nullptr);
	}

	if (workspace != nullptr) {
		QObject::connect(
		    workspace,
		    &QObject::destroyed,
		    this,
		    &I3Monitor::onActiveWorkspaceDestroyed
		);
	}

	this->bActiveWorkspace = workspace;
}

void I3Monitor::onActiveWorkspaceDestroyed() { this->bActiveWorkspace = nullptr; }

} // namespace qs::i3::ipc
