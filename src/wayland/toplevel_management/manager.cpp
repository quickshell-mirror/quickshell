#include "manager.hpp"

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwaylandclientextension.h>

#include "../../core/logcat.hpp"
#include "handle.hpp"
#include "wayland-wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

namespace qs::wayland::toplevel_management::impl {

QS_LOGGING_CATEGORY(logToplevelManagement, "quickshell.wayland.toplevelManagement", QtWarningMsg);

ToplevelManager::ToplevelManager(): QWaylandClientExtensionTemplate(3) { this->initialize(); }

bool ToplevelManager::available() const { return this->isActive(); }

const QVector<ToplevelHandle*>& ToplevelManager::readyToplevels() const {
	return this->mReadyToplevels;
}

ToplevelHandle* ToplevelManager::handleFor(::zwlr_foreign_toplevel_handle_v1* toplevel) {
	if (toplevel == nullptr) return nullptr;

	for (auto* other: this->mToplevels) {
		if (other->object() == toplevel) return other;
	}

	return nullptr;
}

ToplevelManager* ToplevelManager::instance() {
	static auto* instance = new ToplevelManager(); // NOLINT
	return instance;
}

void ToplevelManager::zwlr_foreign_toplevel_manager_v1_toplevel(
    ::zwlr_foreign_toplevel_handle_v1* toplevel
) {
	auto* handle = new ToplevelHandle();
	QObject::connect(handle, &ToplevelHandle::closed, this, &ToplevelManager::onToplevelClosed);
	QObject::connect(handle, &ToplevelHandle::ready, this, &ToplevelManager::onToplevelReady);

	qCDebug(logToplevelManagement) << "Toplevel handle created" << handle;
	this->mToplevels.push_back(handle);

	// Not done in constructor as a close could technically be picked up immediately on init,
	// making touching the handle a UAF.
	handle->init(toplevel);
}

void ToplevelManager::onToplevelReady() {
	auto* handle = qobject_cast<ToplevelHandle*>(this->sender());
	this->mReadyToplevels.push_back(handle);
	emit this->toplevelReady(handle);
}

void ToplevelManager::onToplevelClosed() {
	auto* handle = qobject_cast<ToplevelHandle*>(this->sender());
	this->mReadyToplevels.removeOne(handle);
	this->mToplevels.removeOne(handle);
}

} // namespace qs::wayland::toplevel_management::impl
