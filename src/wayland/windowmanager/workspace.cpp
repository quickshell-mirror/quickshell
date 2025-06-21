#include "workspace.hpp"

#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qproperty.h>

#include "ext_workspace.hpp"

namespace qs::wm::wayland {

WorkspaceManager::WorkspaceManager() {
	auto* impl = impl::WorkspaceManager::instance();

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::serverCommit,
	    this,
	    &WorkspaceManager::onServerCommit
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::workspaceCreated,
	    this,
	    &WorkspaceManager::onWorkspaceCreated
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::workspaceDestroyed,
	    this,
	    &WorkspaceManager::onWorkspaceDestroyed
	);
}

void WorkspaceManager::commit() {
	qCDebug(impl::logWorkspace) << "Committing workspaces";
	impl::WorkspaceManager::instance()->commit();
}

void WorkspaceManager::onServerCommit() {
	for (auto* ws: this->mWorkspaces.valueList()) ws->commitImpl();

	for (auto* wsImpl: this->pendingWorkspaceCreations) {
		auto* ws = new WlWorkspace(this, wsImpl);
		this->workspaceByImpl.insert(wsImpl, ws);
		this->mWorkspaces.insertObject(ws);
	}

	for (auto* wsImpl: this->pendingWorkspaceDestructions) {
		this->mWorkspaces.removeObject(this->workspaceByImpl.value(wsImpl));
		this->workspaceByImpl.remove(wsImpl);
	}

	this->pendingWorkspaceCreations.clear();
	this->pendingWorkspaceDestructions.clear();
}

void WorkspaceManager::onWorkspaceCreated(impl::Workspace* workspace) {
	this->pendingWorkspaceCreations.append(workspace);
}

void WorkspaceManager::onWorkspaceDestroyed(impl::Workspace* workspace) {
	if (!this->pendingWorkspaceCreations.removeOne(workspace)) {
		this->pendingWorkspaceDestructions.append(workspace);
	}
}

WorkspaceManager* WorkspaceManager::instance() {
	static auto* instance = new WorkspaceManager();
	return instance;
}

WlWorkspace::WlWorkspace(WorkspaceManager* manager, impl::Workspace* impl)
    : Workspace(manager)
    , impl(impl) {
	this->commitImpl();
}

void WlWorkspace::commitImpl() {
	Qt::beginPropertyUpdateGroup();
	this->bId = this->impl->id;
	this->bName = this->impl->name;
	this->bActive = this->impl->active;
	this->bShouldDisplay = !this->impl->hidden;
	this->bUrgent = this->impl->urgent;
	this->bCanActivate = this->impl->canActivate;
	this->bCanDeactivate = this->impl->canDeactivate;
	this->bCanSetGroup = this->impl->canAssign;
	Qt::endPropertyUpdateGroup();
}

void WlWorkspace::activate() {
	if (!this->bCanActivate) {
		qCritical(logWorkspace) << this << "cannot be activated";
		return;
	}

	qCDebug(impl::logWorkspace) << "Calling activate() for" << this;
	this->impl->activate();
	WorkspaceManager::commit();
}

void WlWorkspace::deactivate() {
	if (!this->bCanDeactivate) {
		qCritical(logWorkspace) << this << "cannot be deactivated";
		return;
	}

	qCDebug(impl::logWorkspace) << "Calling deactivate() for" << this;
	this->impl->deactivate();
	WorkspaceManager::commit();
}

void WlWorkspace::remove() {
	if (!this->bCanRemove) {
		qCritical(logWorkspace) << this << "cannot be removed";
		return;
	}

	qCDebug(impl::logWorkspace) << "Calling remove() for" << this;
	this->impl->remove();
	WorkspaceManager::commit();
}

} // namespace qs::wm::wayland
