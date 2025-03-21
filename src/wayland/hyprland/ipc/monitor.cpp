#include "monitor.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "workspace.hpp"

namespace qs::hyprland::ipc {

QVariantMap HyprlandMonitor::lastIpcObject() const { return this->mLastIpcObject; }

void HyprlandMonitor::updateInitial(qint32 id, const QString& name, const QString& description) {
	Qt::beginPropertyUpdateGroup();
	this->bId = id;
	this->bName = name;
	this->bDescription = description;
	Qt::endPropertyUpdateGroup();
}

void HyprlandMonitor::updateFromObject(QVariantMap object) {
	auto activeWorkspaceObj = object.value("activeWorkspace").value<QVariantMap>();
	auto activeWorkspaceId = activeWorkspaceObj.value("id").value<qint32>();
	auto activeWorkspaceName = activeWorkspaceObj.value("name").value<QString>();
	auto focused = object.value("focused").value<bool>();

	Qt::beginPropertyUpdateGroup();
	this->bId = object.value("id").value<qint32>();
	this->bName = object.value("name").value<QString>();
	this->bDescription = object.value("description").value<QString>();
	this->bX = object.value("x").value<qint32>();
	this->bY = object.value("y").value<qint32>();
	this->bWidth = object.value("width").value<qint32>();
	this->bHeight = object.value("height").value<qint32>();
	this->bScale = object.value("scale").value<qreal>();
	Qt::endPropertyUpdateGroup();

	if (this->mActiveWorkspace == nullptr
	    || this->mActiveWorkspace->bindableName().value() != activeWorkspaceName)
	{
		auto* workspace = this->ipc->findWorkspaceByName(activeWorkspaceName, true, activeWorkspaceId);
		workspace->setMonitor(this);
		this->setActiveWorkspace(workspace);
	}

	this->mLastIpcObject = std::move(object);
	emit this->lastIpcObjectChanged();

	if (focused) {
		this->ipc->setFocusedMonitor(this);
	}
}

HyprlandWorkspace* HyprlandMonitor::activeWorkspace() const { return this->mActiveWorkspace; }

void HyprlandMonitor::setActiveWorkspace(HyprlandWorkspace* workspace) {
	if (workspace == this->mActiveWorkspace) return;

	if (this->mActiveWorkspace != nullptr) {
		QObject::disconnect(this->mActiveWorkspace, nullptr, this, nullptr);
	}

	this->mActiveWorkspace = workspace;

	if (workspace != nullptr) {
		workspace->setMonitor(this);

		QObject::connect(
		    workspace,
		    &QObject::destroyed,
		    this,
		    &HyprlandMonitor::onActiveWorkspaceDestroyed
		);
	}

	emit this->activeWorkspaceChanged();
}

void HyprlandMonitor::onActiveWorkspaceDestroyed() {
	this->mActiveWorkspace = nullptr;
	emit this->activeWorkspaceChanged();
}

} // namespace qs::hyprland::ipc
