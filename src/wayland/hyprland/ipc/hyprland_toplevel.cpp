#include "hyprland_toplevel.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../toplevel_management/qml.hpp"
#include "connection.hpp"
#include "toplevel_mapping.hpp"
#include "workspace.hpp"

using namespace qs::wayland::toplevel_management;

namespace qs::hyprland::ipc {

HyprlandToplevel::HyprlandToplevel(HyprlandIpc* ipc): QObject(ipc), ipc(ipc) {
	this->bMonitor.setBinding([this]() {
		return this->bWorkspace ? this->bWorkspace->bindableMonitor().value() : nullptr;
	});

	this->bActivated.setBinding([this]() {
		return this->ipc->bindableActiveToplevel().value() == this;
	});

	QObject::connect(
	    this,
	    &HyprlandToplevel::activatedChanged,
	    this,
	    &HyprlandToplevel::onActivatedChanged
	);
}

HyprlandToplevel::HyprlandToplevel(HyprlandIpc* ipc, Toplevel* toplevel): HyprlandToplevel(ipc) {
	this->mWaylandHandle = toplevel->implHandle();
	auto* instance = HyprlandToplevelMappingManager::instance();
	auto addr = instance->getToplevelAddress(this->mWaylandHandle);

	if (!addr) {
		// Address not available, will rely on HyprlandIpc to resolve it.
		return;
	}

	this->setAddress(addr);

	// Check if client is present in HyprlandIPC
	auto* hyprToplevel = ipc->findToplevelByAddress(addr, false);
	// HyprlandIpc will eventually resolve it
	if (!hyprToplevel) return;

	this->setHyprlandHandle(hyprToplevel);
}

void HyprlandToplevel::updateInitial(
    quint64 address,
    const QString& title,
    const QString& workspaceName
) {
	auto* workspace = this->ipc->findWorkspaceByName(workspaceName, false);
	Qt::beginPropertyUpdateGroup();
	this->setAddress(address);
	this->bTitle = title;
	this->setWorkspace(workspace);
	Qt::endPropertyUpdateGroup();
}

void HyprlandToplevel::updateFromObject(const QVariantMap& object) {
	auto addressStr = object.value("address").value<QString>();
	auto title = object.value("title").value<QString>();

	Qt::beginPropertyUpdateGroup();
	bool ok = false;
	auto address = addressStr.toULongLong(&ok, 16);
	if (!ok || !address) {
		return;
	}

	this->setAddress(address);
	this->bTitle = title;

	auto workspaceMap = object.value("workspace").toMap();
	auto workspaceName = workspaceMap.value("name").toString();

	auto* workspace = this->ipc->findWorkspaceByName(workspaceName, false);
	if (!workspace) return;

	this->setWorkspace(workspace);
	this->bLastIpcObject = object;
	Qt::endPropertyUpdateGroup();
}

void HyprlandToplevel::setWorkspace(HyprlandWorkspace* workspace) {
	auto* oldWorkspace = this->bWorkspace.value();
	if (oldWorkspace == workspace) return;

	if (oldWorkspace) {
		QObject::disconnect(oldWorkspace, nullptr, this, nullptr);
	}

	this->bWorkspace = workspace;

	if (workspace) {
		QObject::connect(workspace, &QObject::destroyed, this, [this]() {
			this->bWorkspace = nullptr;
		});
	}
}

void HyprlandToplevel::setAddress(quint64 address) {
	this->mAddress = address;
	emit this->addressChanged();
}

Toplevel* HyprlandToplevel::waylandHandle() {
	return ToplevelManager::instance()->forImpl(this->mWaylandHandle);
}

void HyprlandToplevel::setWaylandHandle(impl::ToplevelHandle* handle) {
	if (this->mWaylandHandle == handle) return;
	if (this->mWaylandHandle) {
		QObject::disconnect(this->mWaylandHandle, nullptr, this, nullptr);
	}

	this->mWaylandHandle = handle;
	if (handle) {
		QObject::connect(handle, &QObject::destroyed, this, [this]() {
			this->mWaylandHandle = nullptr;
		});
	}

	emit this->waylandHandleChanged();
}

void HyprlandToplevel::setHyprlandHandle(HyprlandToplevel* handle) {
	if (this->mHyprlandHandle == handle) return;
	if (this->mHyprlandHandle) {
		QObject::disconnect(this->mHyprlandHandle, nullptr, this, nullptr);
	}
	this->mHyprlandHandle = handle;
	if (handle) {
		QObject::connect(handle, &QObject::destroyed, this, [this]() {
			this->mHyprlandHandle = nullptr;
		});
	}

	emit this->hyprlandHandleChanged();
}

void HyprlandToplevel::onActivatedChanged() {
	if (this->bUrgent.value()) {
		// If was urgent, and now active, clear urgent state
		this->bUrgent = false;
	}
}

HyprlandToplevel* HyprlandToplevel::qmlAttachedProperties(QObject* object) {
	if (auto* toplevel = qobject_cast<Toplevel*>(object)) {
		auto* ipc = HyprlandIpc::instance();
		return new HyprlandToplevel(ipc, toplevel);
	} else {
		return nullptr;
	}
}
} // namespace qs::hyprland::ipc
