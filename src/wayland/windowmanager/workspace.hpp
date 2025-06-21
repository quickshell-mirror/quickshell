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

class WorkspaceManager: public QObject {
	Q_OBJECT;

public:
	static WorkspaceManager* instance();

	ObjectModel<WorkspaceGroup> mWorkspaceGroups {this};
	ObjectModel<WlWorkspace> mWorkspaces {this};

	static void commit();

private slots:
	void onServerCommit();
	void onWorkspaceCreated(impl::Workspace* workspace);
	void onWorkspaceDestroyed(impl::Workspace* workspace);

private:
	WorkspaceManager();

	QList<impl::Workspace*> pendingWorkspaceCreations;
	QList<impl::Workspace*> pendingWorkspaceDestructions;
	QHash<impl::Workspace*, WlWorkspace*> workspaceByImpl;
};

class WlWorkspace: public Workspace {
	Q_OBJECT;

public:
	WlWorkspace(WorkspaceManager* manager, impl::Workspace* impl);

	void commitImpl();

	void activate() override;
	void deactivate() override;
	void remove() override;

private:
	impl::Workspace* impl = nullptr;
};

} // namespace qs::wm::wayland
