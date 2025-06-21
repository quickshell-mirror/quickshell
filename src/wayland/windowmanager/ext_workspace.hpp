#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qscreen.h>
#include <qscreen_platform.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-ext-workspace-v1.h>
#include <qwaylandclientextension.h>
#include <wayland-ext-workspace-v1-client-protocol.h>

#include "../output_tracking.hpp"

namespace qs::wayland::workspace {

Q_DECLARE_LOGGING_CATEGORY(logWorkspace);

class WorkspaceGroup;
class Workspace;

class WorkspaceManager
    : public QWaylandClientExtensionTemplate<WorkspaceManager>
    , public QtWayland::ext_workspace_manager_v1 {
	Q_OBJECT;

public:
	static WorkspaceManager* instance();

	[[nodiscard]] QList<Workspace*> workspaces() { return this->mWorkspaces.values(); }

signals:
	void serverCommit();
	void workspaceCreated(Workspace* workspace);
	void workspaceDestroyed(Workspace* workspace);
	void groupCreated(WorkspaceGroup* group);
	void groupDestroyed(WorkspaceGroup* group);

protected:
	void ext_workspace_manager_v1_workspace_group(::ext_workspace_group_handle_v1* handle) override;
	void ext_workspace_manager_v1_workspace(::ext_workspace_handle_v1* handle) override;
	void ext_workspace_manager_v1_done() override;
	void ext_workspace_manager_v1_finished() override;

private:
	WorkspaceManager();

	void destroyGroup(WorkspaceGroup* group);
	void destroyWorkspace(Workspace* workspace);

	QHash<::ext_workspace_handle_v1*, Workspace*> mWorkspaces;
	QHash<::ext_workspace_group_handle_v1*, WorkspaceGroup*> mGroups;
	QList<WorkspaceGroup*> destroyedGroups;
	QList<Workspace*> destroyedWorkspaces;

	friend class Workspace;
	friend class WorkspaceGroup;
};

class Workspace: public QtWayland::ext_workspace_handle_v1 {
public:
	Workspace(::ext_workspace_handle_v1* handle): QtWayland::ext_workspace_handle_v1(handle) {}
	~Workspace() override;
	Q_DISABLE_COPY_MOVE(Workspace);

	QString id;
	QString name;
	QList<qint32> coordinates;
	WorkspaceGroup* group = nullptr;

	bool active : 1 = false;
	bool urgent : 1 = false;
	bool hidden : 1 = false;

	bool canActivate : 1 = false;
	bool canDeactivate : 1 = false;
	bool canRemove : 1 = false;
	bool canAssign : 1 = false;

protected:
	void ext_workspace_handle_v1_id(const QString& id) override;
	void ext_workspace_handle_v1_name(const QString& name) override;
	void ext_workspace_handle_v1_coordinates(wl_array* coordinates) override;
	void ext_workspace_handle_v1_state(uint32_t state) override;
	void ext_workspace_handle_v1_capabilities(uint32_t capabilities) override;
	void ext_workspace_handle_v1_removed() override;

private:
	void enterGroup(WorkspaceGroup* group);
	void leaveGroup(WorkspaceGroup* group);

	friend class WorkspaceGroup;
};

class WorkspaceGroup: public QtWayland::ext_workspace_group_handle_v1 {
public:
	WorkspaceGroup(::ext_workspace_group_handle_v1* handle)
	    : QtWayland::ext_workspace_group_handle_v1(handle) {}

	~WorkspaceGroup() override;
	Q_DISABLE_COPY_MOVE(WorkspaceGroup);

	WlOutputTracker screens;
	bool canCreateWorkspace : 1 = false;

protected:
	void ext_workspace_group_handle_v1_capabilities(uint32_t capabilities) override;
	void ext_workspace_group_handle_v1_output_enter(::wl_output* output) override;
	void ext_workspace_group_handle_v1_output_leave(::wl_output* output) override;
	void ext_workspace_group_handle_v1_workspace_enter(::ext_workspace_handle_v1* handle) override;
	void ext_workspace_group_handle_v1_workspace_leave(::ext_workspace_handle_v1* handle) override;
	void ext_workspace_group_handle_v1_removed() override;
};

} // namespace qs::wayland::workspace
