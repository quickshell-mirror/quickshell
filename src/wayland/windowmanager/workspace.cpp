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

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::groupCreated,
	    this,
	    &WorkspaceManager::onGroupCreated
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::groupDestroyed,
	    this,
	    &WorkspaceManager::onGroupDestroyed
	);
}

void WorkspaceManager::commit() {
	qCDebug(impl::logWorkspace) << "Committing workspaces";
	impl::WorkspaceManager::instance()->commit();
}

void WorkspaceManager::onServerCommit() {
	// Groups are created/destroyed around workspaces to avoid any nulls making it
	// to the qml engine.

	for (auto* groupImpl: this->pendingGroupCreations) {
		auto* group = new WlWorkspaceGroup(this, groupImpl);
		this->groupsByImpl.insert(groupImpl, group);
		this->mWorkspaceGroups.insertObject(group);
	}

	for (auto* wsImpl: this->pendingWorkspaceCreations) {
		auto* ws = new WlWorkspace(this, wsImpl);
		this->workspaceByImpl.insert(wsImpl, ws);
		this->mWorkspaces.insertObject(ws);
	}

	for (auto* wsImpl: this->pendingWorkspaceDestructions) {
		this->mWorkspaces.removeObject(this->workspaceByImpl.value(wsImpl));
		this->workspaceByImpl.remove(wsImpl);
	}

	for (auto* groupImpl: this->pendingGroupDestructions) {
		this->mWorkspaceGroups.removeObject(this->groupsByImpl.value(groupImpl));
		this->groupsByImpl.remove(groupImpl);
	}

	for (auto* ws: this->mWorkspaces.valueList()) ws->commitImpl();
	for (auto* group: this->mWorkspaceGroups.valueList()) group->commitImpl();

	this->pendingWorkspaceCreations.clear();
	this->pendingWorkspaceDestructions.clear();
	this->pendingGroupCreations.clear();
	this->pendingGroupDestructions.clear();
}

void WorkspaceManager::onWorkspaceCreated(impl::Workspace* workspace) {
	this->pendingWorkspaceCreations.append(workspace);
}

void WorkspaceManager::onWorkspaceDestroyed(impl::Workspace* workspace) {
	if (!this->pendingWorkspaceCreations.removeOne(workspace)) {
		this->pendingWorkspaceDestructions.append(workspace);
	}
}

void WorkspaceManager::onGroupCreated(impl::WorkspaceGroup* group) {
	this->pendingGroupCreations.append(group);
}

void WorkspaceManager::onGroupDestroyed(impl::WorkspaceGroup* group) {
	if (!this->pendingGroupCreations.removeOne(group)) {
		this->pendingGroupDestructions.append(group);
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
	this->bGroup = this->manager()->groupsByImpl.value(this->impl->group);
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

void WlWorkspace::setGroup(WorkspaceGroup* group) {
	if (!this->bCanSetGroup) {
		qCritical(logWorkspace) << this << "cannot be assigned to a group";
		return;
	}

	if (!group) {
		qCritical(logWorkspace) << "Cannot set a workspace's group to null";
		return;
	}

	qCDebug(impl::logWorkspace) << "Assigning" << this << "to" << group;
	// NOLINTNEXTLINE: A WorkspaceGroup will always be a WlWorkspaceGroup under wayland.
	this->impl->assign(static_cast<WlWorkspaceGroup*>(group)->impl->object());
	WorkspaceManager::commit();
}

WlWorkspaceGroup::WlWorkspaceGroup(WorkspaceManager* manager, impl::WorkspaceGroup* impl)
    : WorkspaceGroup(manager)
    , impl(impl) {
	this->commitImpl();
}

void WlWorkspaceGroup::commitImpl() {
	// TODO: will not commit the correct screens if missing qt repr at commit time
	this->bScreens = this->impl->screens.screens();
}

} // namespace qs::wm::wayland
