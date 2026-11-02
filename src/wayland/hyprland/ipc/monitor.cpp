#include "monitor.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

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
	auto* activeWorkspace = this->findWorkspaceFromObject(object.value("activeWorkspace"));
	auto* specialWorkspace = this->findWorkspaceFromObject(object.value("specialWorkspace"));
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

	this->setActiveWorkspace(activeWorkspace);
	this->setSpecialWorkspace(specialWorkspace);

	this->mLastIpcObject = std::move(object);
	emit this->lastIpcObjectChanged();

	if (focused) {
		this->ipc->setFocusedMonitor(this);
	}
}

HyprlandWorkspace* HyprlandMonitor::findWorkspaceFromObject(const QVariant& workspaceObject) {
	auto object = workspaceObject.value<QVariantMap>();
	auto name = object.value("name").value<QString>();
	if (name.isEmpty()) return nullptr;

	auto address = object.value("address").value<QString>();
	if (address.isEmpty()) address = QString::number(object.value("id").value<qint32>());

	return this->ipc->findWorkspaceByName(name, true, address);
}

void HyprlandMonitor::setActiveWorkspace(HyprlandWorkspace* workspace) {
	this->setWorkspaceProperty(
	    &this->bActiveWorkspace,
	    workspace,
	    &HyprlandMonitor::onActiveWorkspaceDestroyed
	);
}

void HyprlandMonitor::setSpecialWorkspace(HyprlandWorkspace* workspace) {
	this->setWorkspaceProperty(
	    &this->bSpecialWorkspace,
	    workspace,
	    &HyprlandMonitor::onSpecialWorkspaceDestroyed
	);
}

void HyprlandMonitor::setWorkspaceProperty(
    QBindable<HyprlandWorkspace*> property,
    HyprlandWorkspace* workspace,
    void (HyprlandMonitor::*onDestroyed)()
) {
	auto* oldWorkspace = property.value();
	if (workspace == oldWorkspace) return;

	if (oldWorkspace != nullptr) {
		QObject::disconnect(oldWorkspace, &QObject::destroyed, this, onDestroyed);
	}

	Qt::beginPropertyUpdateGroup();

	if (workspace != nullptr) {
		workspace->setMonitor(this);
		QObject::connect(workspace, &QObject::destroyed, this, onDestroyed);
	}

	property.setValue(workspace);

	Qt::endPropertyUpdateGroup();
}

void HyprlandMonitor::onActiveWorkspaceDestroyed() { this->bActiveWorkspace = nullptr; }
void HyprlandMonitor::onSpecialWorkspaceDestroyed() { this->bSpecialWorkspace = nullptr; }

} // namespace qs::hyprland::ipc
