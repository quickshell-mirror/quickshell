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

	this->bFocused.setBinding([this]() {
		return HyprlandIpc::instance()->bindableFocusedMonitor().value() == this;
	});

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

	if (this->bActiveWorkspace == nullptr
	    || this->bActiveWorkspace->bindableName().value() != activeWorkspaceName)
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

void HyprlandMonitor::setActiveWorkspace(HyprlandWorkspace* workspace) {
	auto* oldWorkspace = this->bActiveWorkspace.value();
	if (workspace == oldWorkspace) return;

	if (oldWorkspace != nullptr) {
		QObject::disconnect(oldWorkspace, nullptr, this, nullptr);
	}

	Qt::beginPropertyUpdateGroup();

	if (workspace != nullptr) {
		workspace->setMonitor(this);

		QObject::connect(
		    workspace,
		    &QObject::destroyed,
		    this,
		    &HyprlandMonitor::onActiveWorkspaceDestroyed
		);
	}

	this->bActiveWorkspace = workspace;

	Qt::endPropertyUpdateGroup();
}

void HyprlandMonitor::onActiveWorkspaceDestroyed() { this->bActiveWorkspace = nullptr; }

} // namespace qs::hyprland::ipc
