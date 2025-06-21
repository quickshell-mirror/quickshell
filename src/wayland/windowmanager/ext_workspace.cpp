#include "ext_workspace.hpp"

#include <qloggingcategory.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qwayland-ext-workspace-v1.h"
#include "wayland-ext-workspace-v1-client-protocol.h"

namespace qs::wayland::workspace {

Q_LOGGING_CATEGORY(logWorkspace, "quickshell.wm.wayland.workspace");

WorkspaceManager::WorkspaceManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }

WorkspaceManager* WorkspaceManager::instance() {
	static auto* instance = new WorkspaceManager();
	return instance;
}

void WorkspaceManager::ext_workspace_manager_v1_workspace_group(
    ::ext_workspace_group_handle_v1* workspaceGroup
) {
	// todo
}

void WorkspaceManager::ext_workspace_manager_v1_workspace(::ext_workspace_handle_v1* workspace) {
	auto* qws = new Workspace(workspace);
	qCDebug(logWorkspace) << "Created workspace" << qws;
	this->mWorkspaces.insert(workspace, qws);
	emit this->workspaceCreated(qws);
};

void WorkspaceManager::destroyWorkspace(Workspace* workspace) {
	this->destroyedWorkspaces.append(workspace);
	emit this->workspaceDestroyed(workspace);
}

void WorkspaceManager::ext_workspace_manager_v1_done() {
	qCDebug(logWorkspace) << "Workspace changes done";
	emit this->serverCommit();

	for (auto* workspace: this->destroyedWorkspaces) delete workspace;
	this->destroyedWorkspaces.clear();
}

void WorkspaceManager::ext_workspace_manager_v1_finished() {
	// todo
}

WorkspaceGroup::~WorkspaceGroup() { this->destroy(); }
void WorkspaceGroup::ext_workspace_group_handle_v1_removed() { delete this; }

Workspace::~Workspace() {
	if (this->isInitialized()) this->destroy();
}

void Workspace::ext_workspace_handle_v1_id(const QString& id) {
	qCDebug(logWorkspace) << "Updated id for" << this << "to" << id;
	this->id = id;
}

void Workspace::ext_workspace_handle_v1_name(const QString& name) {
	qCDebug(logWorkspace) << "Updated name for" << this << "to" << name;
	this->name = name;
}

void Workspace::ext_workspace_handle_v1_coordinates(wl_array* coordinates) {
	this->coordinates.clear();

	auto* data = static_cast<qint32*>(coordinates->data);
	auto size = static_cast<qsizetype>(coordinates->size / sizeof(qint32));

	for (auto i = 0; i != size; ++i) {
		this->coordinates.append(data[i]); // NOLINT
	}

	qCDebug(logWorkspace) << "Updated coordinates for" << this << "to" << this->coordinates;
}

void Workspace::ext_workspace_handle_v1_state(quint32 state) {
	this->active = state & QtWayland::ext_workspace_handle_v1::state_active;
	this->urgent = state & QtWayland::ext_workspace_handle_v1::state_urgent;
	this->hidden = state & QtWayland::ext_workspace_handle_v1::state_hidden;

	qCDebug(logWorkspace).nospace() << "Updated state for " << this << " to [active: " << this->active
	                                << ", urgent: " << this->urgent << ", hidden: " << this->hidden
	                                << ']';
}

void Workspace::ext_workspace_handle_v1_capabilities(quint32 capabilities) {
	this->canActivate =
	    capabilities & QtWayland::ext_workspace_handle_v1::workspace_capabilities_activate;
	this->canDeactivate =
	    capabilities & QtWayland::ext_workspace_handle_v1::workspace_capabilities_deactivate;
	this->canRemove =
	    capabilities & QtWayland::ext_workspace_handle_v1::workspace_capabilities_remove;
	this->canAssign =
	    capabilities & QtWayland::ext_workspace_handle_v1::workspace_capabilities_assign;

	qCDebug(logWorkspace).nospace() << "Updated capabilities for " << this
	                                << " to [activate: " << this->canActivate
	                                << ", deactivate: " << this->canDeactivate
	                                << ", remove: " << this->canRemove
	                                << ", assign: " << this->canAssign << ']';
}

void Workspace::ext_workspace_handle_v1_removed() {
	qCDebug(logWorkspace) << "Destroyed workspace" << this;
	this->destroy();
	WorkspaceManager::instance()->destroyWorkspace(this);
}

} // namespace qs::wayland::workspace
