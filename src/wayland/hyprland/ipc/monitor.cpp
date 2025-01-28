#include "monitor.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "workspace.hpp"

namespace qs::hyprland::ipc {

qint32 HyprlandMonitor::id() const { return this->mId; }
QString HyprlandMonitor::name() const { return this->mName; }
QString HyprlandMonitor::description() const { return this->mDescription; }
qint32 HyprlandMonitor::x() const { return this->mX; }
qint32 HyprlandMonitor::y() const { return this->mY; }
qint32 HyprlandMonitor::width() const { return this->mWidth; }
qint32 HyprlandMonitor::height() const { return this->mHeight; }
qreal HyprlandMonitor::scale() const { return this->mScale; }
QVariantMap HyprlandMonitor::lastIpcObject() const { return this->mLastIpcObject; }

void HyprlandMonitor::updateInitial(qint32 id, QString name, QString description) {
	if (id != this->mId) {
		this->mId = id;
		emit this->idChanged();
	}

	if (name != this->mName) {
		this->mName = std::move(name);
		emit this->nameChanged();
	}

	if (description != this->mDescription) {
		this->mDescription = std::move(description);
		emit this->descriptionChanged();
	}
}

void HyprlandMonitor::updateFromObject(QVariantMap object) {
	auto id = object.value("id").value<qint32>();
	auto name = object.value("name").value<QString>();
	auto description = object.value("description").value<QString>();
	auto x = object.value("x").value<qint32>();
	auto y = object.value("y").value<qint32>();
	auto width = object.value("width").value<qint32>();
	auto height = object.value("height").value<qint32>();
	auto scale = object.value("height").value<qint32>();
	auto activeWorkspaceObj = object.value("activeWorkspace").value<QVariantMap>();
	auto activeWorkspaceId = activeWorkspaceObj.value("id").value<qint32>();
	auto activeWorkspaceName = activeWorkspaceObj.value("name").value<QString>();
	auto focused = object.value("focused").value<bool>();

	if (id != this->mId) {
		this->mId = id;
		emit this->idChanged();
	}

	if (name != this->mName) {
		this->mName = std::move(name);
		emit this->nameChanged();
	}

	if (description != this->mDescription) {
		this->mDescription = std::move(description);
		emit this->descriptionChanged();
	}

	if (x != this->mX) {
		this->mX = x;
		emit this->xChanged();
	}

	if (y != this->mY) {
		this->mY = y;
		emit this->yChanged();
	}

	if (width != this->mWidth) {
		this->mWidth = width;
		emit this->widthChanged();
	}

	if (height != this->mHeight) {
		this->mHeight = height;
		emit this->heightChanged();
	}

	if (scale != this->mScale) {
		this->mScale = scale;
		emit this->scaleChanged();
	}

	if (this->mActiveWorkspace == nullptr || this->mActiveWorkspace->name() != activeWorkspaceName) {
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
