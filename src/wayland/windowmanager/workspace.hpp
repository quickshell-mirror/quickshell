#pragma once

#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>

#include "../../core/model.hpp"
#include "../../windowmanager/workspace.hpp"
#include "ext_workspace.hpp"

namespace qs::wm::wayland {
namespace impl = qs::wayland::workspace;

class WlWorkspace;
class WlWorkspaceGroup;

class WorkspaceManager: public QObject {
	Q_OBJECT;

public:
	static WorkspaceManager* instance();

	ObjectModel<WlWorkspace> mWorkspaces {this};
	ObjectModel<WlWorkspaceGroup> mWorkspaceGroups {this};

	static void commit();

private slots:
	void onServerCommit();
	void onWorkspaceCreated(impl::Workspace* workspace);
	void onWorkspaceDestroyed(impl::Workspace* workspace);
	void onGroupCreated(impl::WorkspaceGroup* group);
	void onGroupDestroyed(impl::WorkspaceGroup* group);

private:
	WorkspaceManager();

	QList<impl::Workspace*> pendingWorkspaceCreations;
	QList<impl::Workspace*> pendingWorkspaceDestructions;
	QHash<impl::Workspace*, WlWorkspace*> workspaceByImpl;

	QList<impl::WorkspaceGroup*> pendingGroupCreations;
	QList<impl::WorkspaceGroup*> pendingGroupDestructions;
	QHash<impl::WorkspaceGroup*, WlWorkspaceGroup*> groupsByImpl;

	friend class WlWorkspace;
};

class WlWorkspace: public Workspace {
public:
	WlWorkspace(WorkspaceManager* manager, impl::Workspace* impl);

	void commitImpl();

	void activate() override;
	void deactivate() override;
	void remove() override;
	void setGroup(WorkspaceGroup* group) override;

	[[nodiscard]] WorkspaceManager* manager() {
		return static_cast<WorkspaceManager*>(this->parent()); // NOLINT
	}

private:
	impl::Workspace* impl = nullptr;
};

class WlWorkspaceGroup: public WorkspaceGroup {
public:
	WlWorkspaceGroup(WorkspaceManager* manager, impl::WorkspaceGroup* impl);

	void commitImpl();

	[[nodiscard]] WorkspaceManager* manager() {
		return static_cast<WorkspaceManager*>(this->parent()); // NOLINT
	}

private:
	impl::WorkspaceGroup* impl = nullptr;

	friend class WlWorkspace;
};

} // namespace qs::wm::wayland
