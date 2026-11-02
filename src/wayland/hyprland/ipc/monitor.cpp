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

QBindable<HyprlandWorkspace*> HyprlandMonitor::bindableActiveWorkspace() const {
	return &this->bActiveWorkspace;
}

QBindable<HyprlandWorkspace*> HyprlandMonitor::bindableSpecialWorkspace() const {
	return &this->bSpecialWorkspace;
}

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
	auto activeWorkspaceAddress = activeWorkspaceObj.value("address").value<QString>();
	auto activeWorkspaceId = activeWorkspaceObj.value("id").value<qint32>();
	if (activeWorkspaceAddress.isEmpty()) activeWorkspaceAddress = QString::number(activeWorkspaceId);
	auto activeWorkspaceName = activeWorkspaceObj.value("name").value<QString>();

	auto specialWorkspaceObj = object.value("specialWorkspace").value<QVariantMap>();
	auto specialWorkspaceAddress = specialWorkspaceObj.value("address").value<QString>();
	auto specialWorkspaceId = specialWorkspaceObj.value("id").value<qint32>();
	if (specialWorkspaceAddress.isEmpty())
		specialWorkspaceAddress = QString::number(specialWorkspaceId);
	auto specialWorkspaceName = specialWorkspaceObj.value("name").value<QString>();

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
		auto* workspace =
		    this->ipc->findWorkspaceByName(activeWorkspaceName, true, activeWorkspaceAddress);
		workspace->setMonitor(this);
		this->setActiveWorkspace(workspace);
	}

	if (specialWorkspaceName.isEmpty()) {
		this->setSpecialWorkspace(nullptr);
	} else if (
	    this->bSpecialWorkspace == nullptr
	    || this->bSpecialWorkspace->bindableName().value() != specialWorkspaceName
	)
	{
		auto* workspace =
		    this->ipc->findWorkspaceByName(specialWorkspaceName, true, specialWorkspaceAddress);
		this->setSpecialWorkspace(workspace);
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

void HyprlandMonitor::setSpecialWorkspace(HyprlandWorkspace* workspace) {
	auto* oldWorkspace = this->bSpecialWorkspace.value();
	if (workspace == oldWorkspace) return;

	if (oldWorkspace != nullptr) {
		QObject::disconnect(
		    oldWorkspace,
		    &QObject::destroyed,
		    this,
		    &HyprlandMonitor::onSpecialWorkspaceDestroyed
		);
	}

	Qt::beginPropertyUpdateGroup();

	if (workspace != nullptr) {
		workspace->setMonitor(this);

		QObject::connect(
		    workspace,
		    &QObject::destroyed,
		    this,
		    &HyprlandMonitor::onSpecialWorkspaceDestroyed
		);
	}

	this->bSpecialWorkspace = workspace;

	Qt::endPropertyUpdateGroup();
}

void HyprlandMonitor::onSpecialWorkspaceDestroyed() { this->bSpecialWorkspace = nullptr; }

} // namespace qs::hyprland::ipc
