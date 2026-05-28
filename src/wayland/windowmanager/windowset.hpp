#pragma once

#include <qhash.h>
#include <qlist.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>

#include "../../windowmanager/windowset.hpp"
#include "ext_workspace.hpp"

namespace qs::wm::wayland {
namespace impl = qs::wayland::workspace;

class WlWindowset;
class WlWindowsetProjection;

class WindowsetManager: public QObject {
	Q_OBJECT;

public:
	static WindowsetManager* instance();

	void scheduleCommit();

private slots:
	void doCommit();
	void onServerCommit();
	void onWindowsetCreated(impl::Workspace* workspace);
	void onWindowsetDestroyed(impl::Workspace* workspace);
	void onProjectionCreated(impl::WorkspaceGroup* group);
	void onProjectionDestroyed(impl::WorkspaceGroup* group);

private:
	WindowsetManager();

	bool commitScheduled = false;

	QList<impl::Workspace*> pendingWindowsetCreations;
	QList<impl::Workspace*> pendingWindowsetDestructions;
	QHash<impl::Workspace*, WlWindowset*> windowsetByImpl;

	QList<impl::WorkspaceGroup*> pendingProjectionCreations;
	QList<impl::WorkspaceGroup*> pendingProjectionDestructions;
	QHash<impl::WorkspaceGroup*, WlWindowsetProjection*> projectionsByImpl;

	friend class WlWindowset;
};

class WlWindowset: public Windowset {
public:
	WlWindowset(WindowsetManager* manager, impl::Workspace* impl);

	void commitImpl();

	void activate() override;
	void deactivate() override;
	void remove() override;
	void setProjection(WindowsetProjection* projection) override;

	[[nodiscard]] WindowsetManager* manager() {
		return static_cast<WindowsetManager*>(this->parent()); // NOLINT
	}

private:
	impl::Workspace* impl = nullptr;
};

class WlWindowsetProjection: public WindowsetProjection {
public:
	WlWindowsetProjection(WindowsetManager* manager, impl::WorkspaceGroup* impl);

	void commitImpl();

	[[nodiscard]] WindowsetManager* manager() {
		return static_cast<WindowsetManager*>(this->parent()); // NOLINT
	}

private:
	impl::WorkspaceGroup* impl = nullptr;

	friend class WlWindowset;
};

} // namespace qs::wm::wayland
