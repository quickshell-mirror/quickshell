#include "window.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "connection.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

NiriWindow::NiriWindow(NiriIpc* ipc): QObject(ipc), ipc(ipc) {}

void NiriWindow::updateFromObject(const QVariantMap& object) {
	Qt::beginPropertyUpdateGroup();

	// ID cannot be updated after creation.
	if (this->bId.value() == 0) {
		this->bId = object.value("id").value<qint64>();
	}

	this->bTitle = object.value("title").value<QString>();
	this->bAppId = object.value("app_id").value<QString>();
	this->bPid = object.value("pid").value<qint32>();
	this->bFocused = object.value("is_focused").value<bool>();
	this->bFloating = object.value("is_floating").value<bool>();
	this->bUrgent = object.value("is_urgent").value<bool>();

	// workspace_id may reference a workspace that was already removed, as niri
	// events are not always atomic. See NiriIpc::onEvent.
	auto workspaceId = object.value("workspace_id").value<qint64>();
	auto* currentWorkspace = this->bWorkspace.value();

	if (workspaceId == 0) {
		this->setWorkspace(nullptr);
	} else if (currentWorkspace == nullptr || currentWorkspace->bindableId().value() != workspaceId) {
		this->setWorkspace(this->ipc->findWorkspaceById(workspaceId));
	}

	this->mLastIpcObject = object;
	emit this->lastIpcObjectChanged();

	this->mLayout = object.value("layout").toMap();
	emit this->layoutChanged();

	this->mFocusTimestamp = object.value("focus_timestamp").toMap();
	emit this->focusTimestampChanged();

	Qt::endPropertyUpdateGroup();
}

void NiriWindow::setLayout(const QVariantMap& layout) {
	this->mLayout = layout;
	emit this->layoutChanged();
}

void NiriWindow::setFocusTimestamp(const QVariantMap& focusTimestamp) {
	this->mFocusTimestamp = focusTimestamp;
	emit this->focusTimestampChanged();
}

void NiriWindow::setWorkspace(NiriWorkspace* workspace) {
	auto* oldWorkspace = this->bWorkspace.value();
	if (workspace == oldWorkspace) return;

	if (oldWorkspace != nullptr) {
		QObject::disconnect(oldWorkspace, nullptr, this, nullptr);
	}

	if (workspace != nullptr) {
		QObject::connect(workspace, &QObject::destroyed, this, &NiriWindow::onWorkspaceDestroyed);
	}

	this->bWorkspace = workspace;
}

void NiriWindow::onWorkspaceDestroyed() { this->bWorkspace = nullptr; }

QVariantMap NiriWindow::lastIpcObject() const { return this->mLastIpcObject; }

QVariantMap NiriWindow::layout() const { return this->mLayout; }

QVariantMap NiriWindow::focusTimestamp() const { return this->mFocusTimestamp; }

void NiriWindow::activate() {
	this->ipc->dispatch(QString(R"({"FocusWindow":{"id":%1}})").arg(this->bId.value()));
}

} // namespace qs::niri::ipc
