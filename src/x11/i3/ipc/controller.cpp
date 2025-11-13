#include "controller.hpp"
#include <algorithm>

#include <qbytearray.h>
#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qdatastream.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qsysinfo.h>
#include <qtenvironmentvariables.h>
#include <qtypes.h>

#include "../../../core/logcat.hpp"
#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"
#include "monitor.hpp"
#include "workspace.hpp"

namespace qs::i3::ipc {

namespace {
QS_LOGGING_CATEGORY(logI3Ipc, "quickshell.I3.ipc", QtWarningMsg);
QS_LOGGING_CATEGORY(logI3IpcEvents, "quickshell.I3.ipc.events", QtWarningMsg);
} // namespace

I3IpcController::I3IpcController(): I3Ipc({"workspace", "output"}) {
	// bind focused workspace to focused monitor's active workspace
	this->bFocusedWorkspace.setBinding([this]() -> I3Workspace* {
		if (!this->bFocusedMonitor) return nullptr;
		return this->bFocusedMonitor->bindableActiveWorkspace().value();
	});

	// clang-format off
	QObject::connect(this, &I3Ipc::rawEvent, this, &I3IpcController::onEvent);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::connected, this, &I3IpcController::onConnected);
	// clang-format on
}

void I3IpcController::onConnected() {
	// Workspaces must be refreshed before monitors or no focus will be
	// detected on launch.
	this->refreshWorkspaces();
	this->refreshMonitors();
}

void I3IpcController::setFocusedMonitor(I3Monitor* monitor) {
	auto* oldMonitor = this->bFocusedMonitor.value();
	if (monitor == oldMonitor) return;

	if (oldMonitor != nullptr) {
		QObject::disconnect(oldMonitor, nullptr, this, nullptr);
	}

	if (monitor != nullptr) {
		QObject::connect(
		    monitor,
		    &QObject::destroyed,
		    this,
		    &I3IpcController::onFocusedMonitorDestroyed
		);
	}

	this->bFocusedMonitor = monitor;
}

void I3IpcController::onFocusedMonitorDestroyed() { this->bFocusedMonitor = nullptr; }

I3IpcController* I3IpcController::instance() {
	static I3IpcController* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new I3IpcController();
		instance->connect();
	}

	return instance;
}

void I3IpcController::refreshWorkspaces() {
	this->makeRequest(I3Ipc::buildRequestMessage(EventCode::GetWorkspaces));
}

void I3IpcController::handleGetWorkspacesEvent(I3IpcEvent* event) {
	auto data = event->mData;

	auto workspaces = data.array();

	const auto& mList = this->mWorkspaces.valueList();
	auto names = QVector<QString>();

	qCDebug(logI3Ipc) << "There are" << workspaces.toVariantList().length() << "workspaces";
	for (auto entry: workspaces) {
		auto object = entry.toObject().toVariantMap();
		auto name = object["name"].toString();

		auto workspaceIter = std::ranges::find_if(mList, [name](I3Workspace* m) {
			return m->bindableName().value() == name;
		});

		auto* workspace = workspaceIter == mList.end() ? nullptr : *workspaceIter;
		auto existed = workspace != nullptr;

		if (workspace == nullptr) {
			workspace = new I3Workspace(this);
		}

		workspace->updateFromObject(object);

		if (!existed) {
			this->mWorkspaces.insertObjectSorted(workspace, &I3IpcController::compareWorkspaces);
		}

		if (!this->bFocusedWorkspace && object.value("focused").value<bool>()) {
			this->bFocusedMonitor = workspace->bindableMonitor().value();
		}

		names.push_back(name);
	}

	auto removedWorkspaces = QVector<I3Workspace*>();

	for (auto* workspace: mList) {
		if (!names.contains(workspace->bindableName().value())) {
			removedWorkspaces.push_back(workspace);
		}
	}

	qCDebug(logI3Ipc) << "Removing" << removedWorkspaces.length() << "deleted workspaces.";

	for (auto* workspace: removedWorkspaces) {
		this->mWorkspaces.removeObject(workspace);
		delete workspace;
	}
}

void I3IpcController::refreshMonitors() {
	this->makeRequest(I3Ipc::buildRequestMessage(EventCode::GetOutputs));
}

void I3IpcController::handleGetOutputsEvent(I3IpcEvent* event) {
	auto data = event->mData;

	auto monitors = data.array();
	const auto& mList = this->mMonitors.valueList();
	auto names = QVector<QString>();

	qCDebug(logI3Ipc) << "There are" << monitors.toVariantList().length() << "monitors";

	for (auto elem: monitors) {
		auto object = elem.toObject().toVariantMap();
		auto name = object["name"].toString();

		auto monitorIter = std::ranges::find_if(mList, [name](I3Monitor* m) {
			return m->bindableName().value() == name;
		});

		auto* monitor = monitorIter == mList.end() ? nullptr : *monitorIter;
		auto existed = monitor != nullptr;

		if (monitor == nullptr) {
			monitor = new I3Monitor(this);
		}

		monitor->updateFromObject(object);

		if (monitor->bindableFocused().value()) {
			this->setFocusedMonitor(monitor);
		}

		if (!existed) {
			this->mMonitors.insertObject(monitor);
		}

		names.push_back(name);
	}

	auto removedMonitors = QVector<I3Monitor*>();

	for (auto* monitor: mList) {
		if (!names.contains(monitor->bindableName().value())) {
			removedMonitors.push_back(monitor);
		}
	}

	qCDebug(logI3Ipc) << "Removing" << removedMonitors.length() << "disconnected monitors.";

	for (auto* monitor: removedMonitors) {
		this->mMonitors.removeObject(monitor);
		delete monitor;
	}
}

void I3IpcController::onEvent(I3IpcEvent* event) {
	switch (event->mCode) {
	case EventCode::Workspace: this->handleWorkspaceEvent(event); return;
	case EventCode::Output:
		/// I3 only sends an "unspecified" event, so we have to query the data changes ourselves
		qCInfo(logI3Ipc) << "Refreshing Monitors...";
		this->refreshMonitors();
		return;
	case EventCode::Subscribe: qCInfo(logI3Ipc) << "Connected to IPC"; return;
	case EventCode::GetOutputs: this->handleGetOutputsEvent(event); return;
	case EventCode::GetWorkspaces: this->handleGetWorkspacesEvent(event); return;
	case EventCode::RunCommand: I3IpcController::handleRunCommand(event); return;
	case EventCode::Unknown:
		qCWarning(logI3Ipc) << "Unknown event:" << event->type() << event->data();
		return;
	default: qCWarning(logI3Ipc) << "Unhandled event:" << event->type();
	}
}

void I3IpcController::handleRunCommand(I3IpcEvent* event) {
	for (auto r: event->mData.array()) {
		auto obj = r.toObject();
		const bool success = obj["success"].toBool();

		if (!success) {
			const QString error = obj["error"].toString();
			qCWarning(logI3Ipc) << "Error occured while running command:" << error;
		}
	}
}

void I3IpcController::handleWorkspaceEvent(I3IpcEvent* event) {
	// If a workspace doesn't exist, and is being switch to, no focus change event is emited,
	// only the init one, which does not contain the previously focused workspace
	auto change = event->mData["change"];

	if (change == "init") {
		qCInfo(logI3IpcEvents) << "New workspace has been created";

		auto workspaceData = event->mData["current"];

		auto* workspace = this->findWorkspaceByID(workspaceData["id"].toInt(-1));
		auto existed = workspace != nullptr;

		if (!existed) {
			workspace = new I3Workspace(this);
		}

		if (workspaceData.isObject()) {
			workspace->updateFromObject(workspaceData.toObject().toVariantMap());
		}

		if (!existed) {
			this->mWorkspaces.insertObjectSorted(workspace, &I3IpcController::compareWorkspaces);
			qCInfo(logI3Ipc) << "Added workspace" << workspace->bindableName().value() << "to list";
		}
	} else if (change == "focus") {
		auto oldData = event->mData["old"];
		auto newData = event->mData["current"];
		auto oldName = oldData["name"].toString();
		auto newName = newData["name"].toString();

		qCInfo(logI3IpcEvents) << "Focus changed: " << oldName << "->" << newName;

		if (auto* oldWorkspace = this->findWorkspaceByName(oldName)) {
			oldWorkspace->updateFromObject(oldData.toObject().toVariantMap());
		}

		auto* newWorkspace = this->findWorkspaceByName(newName);

		if (newWorkspace == nullptr) {
			newWorkspace = new I3Workspace(this);
		}

		newWorkspace->updateFromObject(newData.toObject().toVariantMap());

		if (newWorkspace->bindableMonitor().value()) {
			auto* monitor = newWorkspace->bindableMonitor().value();
			monitor->setFocusedWorkspace(newWorkspace);
			this->bFocusedMonitor = monitor;
		}
	} else if (change == "empty") {
		auto name = event->mData["current"]["name"].toString();

		auto* oldWorkspace = this->findWorkspaceByName(name);

		if (oldWorkspace != nullptr) {
			qCInfo(logI3Ipc) << "Deleting" << oldWorkspace->bindableId().value() << name;

			if (this->bFocusedWorkspace == oldWorkspace) {
				this->bFocusedMonitor->setFocusedWorkspace(nullptr);
			}

			this->workspaces()->removeObject(oldWorkspace);

			delete oldWorkspace;
		} else {
			qCInfo(logI3Ipc) << "Workspace" << name << "has already been deleted";
		}
	} else if (change == "move" || change == "rename" || change == "urgent") {
		auto name = event->mData["current"]["name"].toString();

		auto* workspace = this->findWorkspaceByName(name);

		if (workspace != nullptr) {
			auto data = event->mData["current"].toObject().toVariantMap();

			workspace->updateFromObject(data);
		} else {
			qCWarning(logI3Ipc) << "Workspace" << name << "doesn't exist";
		}
	} else if (change == "reload") {
		qCInfo(logI3Ipc) << "Refreshing Workspaces...";
		this->refreshWorkspaces();
	}
}

I3Monitor* I3IpcController::monitorFor(QuickshellScreenInfo* screen) {
	if (screen == nullptr) return nullptr;

	return this->findMonitorByName(screen->name());
}

I3Workspace* I3IpcController::findWorkspaceByID(qint32 id) {
	auto list = this->mWorkspaces.valueList();
	auto workspaceIter =
	    std::ranges::find_if(list, [id](I3Workspace* m) { return m->bindableId().value() == id; });

	return workspaceIter == list.end() ? nullptr : *workspaceIter;
}

I3Workspace* I3IpcController::findWorkspaceByName(const QString& name) {
	auto list = this->mWorkspaces.valueList();
	auto workspaceIter = std::ranges::find_if(list, [name](I3Workspace* m) {
		return m->bindableName().value() == name;
	});

	return workspaceIter == list.end() ? nullptr : *workspaceIter;
}

I3Monitor* I3IpcController::findMonitorByName(const QString& name, bool createIfMissing) {
	auto list = this->mMonitors.valueList();
	auto monitorIter = std::ranges::find_if(list, [name](I3Monitor* m) {
		return m->bindableName().value() == name;
	});

	if (monitorIter != list.end()) {
		return *monitorIter;
	} else if (createIfMissing) {
		qCDebug(logI3Ipc) << "Monitor" << name << "requested before creation, performing early init";
		auto* monitor = new I3Monitor(this);
		monitor->updateInitial(name);
		this->mMonitors.insertObject(monitor);
		return monitor;
	} else {
		return nullptr;
	}
}

ObjectModel<I3Monitor>* I3IpcController::monitors() { return &this->mMonitors; }
ObjectModel<I3Workspace>* I3IpcController::workspaces() { return &this->mWorkspaces; }

bool I3IpcController::compareWorkspaces(I3Workspace* a, I3Workspace* b) {
	return a->bindableNumber().value() > b->bindableNumber().value();
}

} // namespace qs::i3::ipc
