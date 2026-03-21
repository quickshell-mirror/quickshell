#include "windowset.hpp"

#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qproperty.h>

#include "../../windowmanager/screenprojection.hpp"
#include "../../windowmanager/windowmanager.hpp"
#include "../../windowmanager/windowset.hpp"
#include "ext_workspace.hpp"

namespace qs::wm::wayland {

WindowsetManager::WindowsetManager() {
	auto* impl = impl::WorkspaceManager::instance();

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::serverCommit,
	    this,
	    &WindowsetManager::onServerCommit
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::workspaceCreated,
	    this,
	    &WindowsetManager::onWindowsetCreated
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::workspaceDestroyed,
	    this,
	    &WindowsetManager::onWindowsetDestroyed
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::groupCreated,
	    this,
	    &WindowsetManager::onProjectionCreated
	);

	QObject::connect(
	    impl,
	    &impl::WorkspaceManager::groupDestroyed,
	    this,
	    &WindowsetManager::onProjectionDestroyed
	);
}

void WindowsetManager::scheduleCommit() {
	if (this->commitScheduled) {
		qCDebug(impl::logWorkspace) << "Workspace commit already scheduled.";
		return;
	}

	qCDebug(impl::logWorkspace) << "Scheduling workspace commit...";
	this->commitScheduled = true;
	QMetaObject::invokeMethod(this, &WindowsetManager::doCommit, Qt::QueuedConnection);
}

void WindowsetManager::doCommit() { // NOLINT
	qCDebug(impl::logWorkspace) << "Committing workspaces...";
	impl::WorkspaceManager::instance()->commit();
	this->commitScheduled = false;
}

void WindowsetManager::onServerCommit() {
	// Projections are created/destroyed around windowsets to avoid any nulls making it
	// to the qml engine.

	Qt::beginPropertyUpdateGroup();

	auto* wm = WindowManager::instance();
	auto windowsets = wm->bWindowsets.value();
	auto projections = wm->bWindowsetProjections.value();

	for (auto* projImpl: this->pendingProjectionCreations) {
		auto* projection = new WlWindowsetProjection(this, projImpl);
		this->projectionsByImpl.insert(projImpl, projection);
		projections.append(projection);
	}

	for (auto* wsImpl: this->pendingWindowsetCreations) {
		auto* ws = new WlWindowset(this, wsImpl);
		this->windowsetByImpl.insert(wsImpl, ws);
		windowsets.append(ws);
	}

	for (auto* wsImpl: this->pendingWindowsetDestructions) {
		windowsets.removeOne(this->windowsetByImpl.value(wsImpl));
		this->windowsetByImpl.remove(wsImpl);
	}

	for (auto* projImpl: this->pendingProjectionDestructions) {
		projections.removeOne(this->projectionsByImpl.value(projImpl));
		this->projectionsByImpl.remove(projImpl);
	}

	for (auto* ws: windowsets) {
		static_cast<WlWindowset*>(ws)->commitImpl(); // NOLINT
	}

	for (auto* projection: projections) {
		static_cast<WlWindowsetProjection*>(projection)->commitImpl(); // NOLINT
	}

	this->pendingWindowsetCreations.clear();
	this->pendingWindowsetDestructions.clear();
	this->pendingProjectionCreations.clear();
	this->pendingProjectionDestructions.clear();

	wm->bWindowsets = windowsets;
	wm->bWindowsetProjections = projections;

	Qt::endPropertyUpdateGroup();
}

void WindowsetManager::onWindowsetCreated(impl::Workspace* workspace) {
	this->pendingWindowsetCreations.append(workspace);
}

void WindowsetManager::onWindowsetDestroyed(impl::Workspace* workspace) {
	if (!this->pendingWindowsetCreations.removeOne(workspace)) {
		this->pendingWindowsetDestructions.append(workspace);
	}
}

void WindowsetManager::onProjectionCreated(impl::WorkspaceGroup* group) {
	this->pendingProjectionCreations.append(group);
}

void WindowsetManager::onProjectionDestroyed(impl::WorkspaceGroup* group) {
	if (!this->pendingProjectionCreations.removeOne(group)) {
		this->pendingProjectionDestructions.append(group);
	}
}

WindowsetManager* WindowsetManager::instance() {
	static auto* instance = new WindowsetManager();
	return instance;
}

WlWindowset::WlWindowset(WindowsetManager* manager, impl::Workspace* impl)
    : Windowset(manager)
    , impl(impl) {
	this->commitImpl();
}

void WlWindowset::commitImpl() {
	Qt::beginPropertyUpdateGroup();
	this->bId = this->impl->id;
	this->bName = this->impl->name;
	this->bCoordinates = this->impl->coordinates;
	this->bActive = this->impl->active;
	this->bShouldDisplay = !this->impl->hidden;
	this->bUrgent = this->impl->urgent;
	this->bCanActivate = this->impl->canActivate;
	this->bCanDeactivate = this->impl->canDeactivate;
	this->bCanSetProjection = this->impl->canAssign;
	this->bProjection = this->manager()->projectionsByImpl.value(this->impl->group);
	Qt::endPropertyUpdateGroup();
}

void WlWindowset::activate() {
	if (!this->bCanActivate) {
		qCritical(logWorkspace) << this << "cannot be activated";
		return;
	}

	qCDebug(impl::logWorkspace) << "Calling activate() for" << this;
	this->impl->activate();
	WindowsetManager::instance()->scheduleCommit();
}

void WlWindowset::deactivate() {
	if (!this->bCanDeactivate) {
		qCritical(logWorkspace) << this << "cannot be deactivated";
		return;
	}

	qCDebug(impl::logWorkspace) << "Calling deactivate() for" << this;
	this->impl->deactivate();
	WindowsetManager::instance()->scheduleCommit();
}

void WlWindowset::remove() {
	if (!this->bCanRemove) {
		qCritical(logWorkspace) << this << "cannot be removed";
		return;
	}

	qCDebug(impl::logWorkspace) << "Calling remove() for" << this;
	this->impl->remove();
	WindowsetManager::instance()->scheduleCommit();
}

void WlWindowset::setProjection(WindowsetProjection* projection) {
	if (!this->bCanSetProjection) {
		qCritical(logWorkspace) << this << "cannot be assigned to a projection";
		return;
	}

	if (!projection) {
		qCritical(logWorkspace) << "Cannot set a windowset's projection to null";
		return;
	}

	WlWindowsetProjection* wlProjection = nullptr;
	if (auto* p = dynamic_cast<WlWindowsetProjection*>(projection)) {
		wlProjection = p;
	} else if (auto* p = dynamic_cast<ScreenProjection*>(projection)) {
		// In the 99% case, there will only be a single windowset on a screen.
		// In the 1% case, the oldest projection (first in list) is most likely the desired one.
		auto* screen = p->screen();
		for (const auto& proj: WindowsetManager::instance()->projectionsByImpl.values()) {
			if (proj->bQScreens.value().contains(screen)) {
				wlProjection = proj;
				break;
			}
		}
	}

	if (!wlProjection) {
		qCritical(logWorkspace) << "Cannot set a windowset's projection to" << projection
		                        << "as no wayland projection could be derived.";
		return;
	}

	qCDebug(impl::logWorkspace) << "Assigning" << this << "to" << projection;
	this->impl->assign(wlProjection->impl->object());
	WindowsetManager::instance()->scheduleCommit();
}

WlWindowsetProjection::WlWindowsetProjection(WindowsetManager* manager, impl::WorkspaceGroup* impl)
    : WindowsetProjection(manager)
    , impl(impl) {
	this->commitImpl();
}

void WlWindowsetProjection::commitImpl() {
	// TODO: will not commit the correct screens if missing qt repr at commit time
	this->bQScreens = this->impl->screens.screens();
}

} // namespace qs::wm::wayland
