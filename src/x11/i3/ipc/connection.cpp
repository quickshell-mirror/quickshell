#include <algorithm>
#include <array>
#include <cstring>
#include <tuple>

#include <bit>
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
#include <qnamespace.h>
#include <qobject.h>
#include <qsysinfo.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
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

void I3Ipc::makeRequest(const QByteArray& request) {
	if (!this->valid) {
		qCWarning(logI3IpcEvents) << "IPC connection is not open, ignoring request.";
		return;
	}
	this->liveEventSocket.write(request);
	this->liveEventSocket.flush();
}

void I3Ipc::dispatch(const QString& payload) {
	auto message = I3Ipc::buildRequestMessage(EventCode::RunCommand, payload.toLocal8Bit());

	this->makeRequest(message);
}

QByteArray I3Ipc::buildRequestMessage(EventCode cmd, const QByteArray& payload) {
	auto payloadLength = static_cast<quint32>(payload.length());

	auto type = QByteArray(std::bit_cast<std::array<char, 4>>(cmd).data(), 4);
	auto len = QByteArray(std::bit_cast<std::array<char, 4>>(payloadLength).data(), 4);

	return MAGIC.data() + len + type + payload;
}

I3Ipc::I3Ipc() {
	auto sock = qEnvironmentVariable("I3SOCK");

	if (sock.isEmpty()) {
		qCWarning(logI3Ipc) << "$I3SOCK is unset. Trying $SWAYSOCK.";

		sock = qEnvironmentVariable("SWAYSOCK");

		if (sock.isEmpty()) {
			qCWarning(logI3Ipc) << "$SWAYSOCK and I3SOCK are unset. Cannot connect to socket.";
			return;
		}
	}

	this->bFocusedWorkspace.setBinding([this]() -> I3Workspace* {
		if (!this->bFocusedMonitor) return nullptr;
		return this->bFocusedMonitor->bindableActiveWorkspace().value();
	});

	this->mSocketPath = sock;

	// clang-format off
	QObject::connect(&this->liveEventSocket, &QLocalSocket::errorOccurred, this, &I3Ipc::eventSocketError);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::stateChanged, this, &I3Ipc::eventSocketStateChanged);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::readyRead, this, &I3Ipc::eventSocketReady);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::connected, this, &I3Ipc::subscribe);
	// clang-format on

	this->liveEventSocketDs.setDevice(&this->liveEventSocket);
	this->liveEventSocketDs.setByteOrder(static_cast<QDataStream::ByteOrder>(QSysInfo::ByteOrder));

	this->liveEventSocket.connectToServer(this->mSocketPath);
}

void I3Ipc::subscribe() {
	auto payload = QByteArray(R"(["workspace","output"])");
	auto message = I3Ipc::buildRequestMessage(EventCode::Subscribe, payload);

	this->makeRequest(message);

	// Workspaces must be refreshed before monitors or no focus will be
	// detected on launch.
	this->refreshWorkspaces();
	this->refreshMonitors();
}

void I3Ipc::eventSocketReady() {
	for (auto& [type, data]: this->parseResponse()) {
		this->event.mCode = type;
		this->event.mData = data;

		this->onEvent(&this->event);
		emit this->rawEvent(&this->event);
	}
}

void I3Ipc::reconnectIPC() {
	qCWarning(logI3Ipc) << "Fatal IPC error occured, recreating connection";
	this->liveEventSocket.disconnectFromServer();
	this->liveEventSocket.connectToServer(this->mSocketPath);
}

QVector<Event> I3Ipc::parseResponse() {
	QVector<std::tuple<EventCode, QJsonDocument>> events;
	const int magicLen = 6;

	while (!this->liveEventSocketDs.atEnd()) {
		this->liveEventSocketDs.startTransaction();
		this->liveEventSocketDs.startTransaction();

		std::array<char, 6> buffer = {};
		qint32 size = 0;
		qint32 type = EventCode::Unknown;

		this->liveEventSocketDs.readRawData(buffer.data(), magicLen);
		this->liveEventSocketDs >> size;
		this->liveEventSocketDs >> type;

		if (!this->liveEventSocketDs.commitTransaction()) break;

		QByteArray payload(size, Qt::Uninitialized);

		this->liveEventSocketDs.readRawData(payload.data(), size);

		if (!this->liveEventSocketDs.commitTransaction()) break;

		if (strncmp(buffer.data(), MAGIC.data(), 6) != 0) {
			qCWarning(logI3Ipc) << "No magic sequence found in string.";
			this->reconnectIPC();
			break;
		};

		if (I3IpcEvent::intToEvent(type) == EventCode::Unknown) {
			qCWarning(logI3Ipc) << "Received unknown event";
			break;
		}

		// Importing this makes CI builds fail for some reason.
		QJsonParseError e; // NOLINT (misc-include-cleaner)

		auto data = QJsonDocument::fromJson(payload, &e);
		if (e.error != QJsonParseError::NoError) {
			qCWarning(logI3Ipc) << "Invalid JSON value:" << e.errorString();
			break;
		} else {
			events.push_back(std::tuple(I3IpcEvent::intToEvent(type), data));
		}
	}

	return events;
}

void I3Ipc::eventSocketError(QLocalSocket::LocalSocketError error) const {
	if (!this->valid) {
		qCWarning(logI3Ipc) << "Unable to connect to I3 socket:" << error;
	} else {
		qCWarning(logI3Ipc) << "I3 socket error:" << error;
	}
}

void I3Ipc::eventSocketStateChanged(QLocalSocket::LocalSocketState state) {
	if (state == QLocalSocket::ConnectedState) {
		qCInfo(logI3Ipc) << "I3 event socket connected.";
		emit this->connected();
	} else if (state == QLocalSocket::UnconnectedState && this->valid) {
		qCWarning(logI3Ipc) << "I3 event socket disconnected.";
	}

	this->valid = state == QLocalSocket::ConnectedState;
}

QString I3Ipc::socketPath() const { return this->mSocketPath; }

void I3Ipc::setFocusedMonitor(I3Monitor* monitor) {
	auto* oldMonitor = this->bFocusedMonitor.value();
	if (monitor == oldMonitor) return;

	if (oldMonitor != nullptr) {
		QObject::disconnect(oldMonitor, nullptr, this, nullptr);
	}

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &I3Ipc::onFocusedMonitorDestroyed);
	}

	this->bFocusedMonitor = monitor;
}

void I3Ipc::onFocusedMonitorDestroyed() { this->bFocusedMonitor = nullptr; }

I3Ipc* I3Ipc::instance() {
	static I3Ipc* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new I3Ipc();
	}

	return instance;
}

void I3Ipc::refreshWorkspaces() {
	this->makeRequest(I3Ipc::buildRequestMessage(EventCode::GetWorkspaces));
}

void I3Ipc::handleGetWorkspacesEvent(I3IpcEvent* event) {
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
			this->mWorkspaces.insertObjectSorted(workspace, &I3Ipc::compareWorkspaces);
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

void I3Ipc::refreshMonitors() {
	this->makeRequest(I3Ipc::buildRequestMessage(EventCode::GetOutputs));
}

void I3Ipc::handleGetOutputsEvent(I3IpcEvent* event) {
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

void I3Ipc::onEvent(I3IpcEvent* event) {
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
	case EventCode::RunCommand: I3Ipc::handleRunCommand(event); return;
	case EventCode::Unknown:
		qCWarning(logI3Ipc) << "Unknown event:" << event->type() << event->data();
		return;
	default: qCWarning(logI3Ipc) << "Unhandled event:" << event->type();
	}
}

void I3Ipc::handleRunCommand(I3IpcEvent* event) {
	for (auto r: event->mData.array()) {
		auto obj = r.toObject();
		const bool success = obj["success"].toBool();

		if (!success) {
			const QString error = obj["error"].toString();
			qCWarning(logI3Ipc) << "Error occured while running command:" << error;
		}
	}
}

void I3Ipc::handleWorkspaceEvent(I3IpcEvent* event) {
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
			this->mWorkspaces.insertObjectSorted(workspace, &I3Ipc::compareWorkspaces);
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

I3Monitor* I3Ipc::monitorFor(QuickshellScreenInfo* screen) {
	if (screen == nullptr) return nullptr;

	return this->findMonitorByName(screen->name());
}

I3Workspace* I3Ipc::findWorkspaceByID(qint32 id) {
	auto list = this->mWorkspaces.valueList();
	auto workspaceIter =
	    std::ranges::find_if(list, [id](I3Workspace* m) { return m->bindableId().value() == id; });

	return workspaceIter == list.end() ? nullptr : *workspaceIter;
}

I3Workspace* I3Ipc::findWorkspaceByName(const QString& name) {
	auto list = this->mWorkspaces.valueList();
	auto workspaceIter = std::ranges::find_if(list, [name](I3Workspace* m) {
		return m->bindableName().value() == name;
	});

	return workspaceIter == list.end() ? nullptr : *workspaceIter;
}

I3Monitor* I3Ipc::findMonitorByName(const QString& name, bool createIfMissing) {
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

ObjectModel<I3Monitor>* I3Ipc::monitors() { return &this->mMonitors; }
ObjectModel<I3Workspace>* I3Ipc::workspaces() { return &this->mWorkspaces; }

bool I3Ipc::compareWorkspaces(I3Workspace* a, I3Workspace* b) {
	return a->bindableNumber().value() > b->bindableNumber().value();
}

QString I3IpcEvent::type() const { return I3IpcEvent::eventToString(this->mCode); }
QString I3IpcEvent::data() const { return QString::fromUtf8(this->mData.toJson()); }

EventCode I3IpcEvent::intToEvent(quint32 raw) {
	if ((EventCode::Workspace <= raw && raw <= EventCode::Input)
	    || (EventCode::RunCommand <= raw && raw <= EventCode::GetTree))
	{
		return static_cast<EventCode>(raw);
	} else {
		return EventCode::Unknown;
	}
}

QString I3IpcEvent::eventToString(EventCode event) {
	switch (event) {
	case EventCode::RunCommand: return "run_command"; break;
	case EventCode::GetWorkspaces: return "get_workspaces"; break;
	case EventCode::Subscribe: return "subscribe"; break;
	case EventCode::GetOutputs: return "get_outputs"; break;
	case EventCode::GetTree: return "get_tree"; break;

	case EventCode::Output: return "output"; break;
	case EventCode::Workspace: return "workspace"; break;
	case EventCode::Mode: return "mode"; break;
	case EventCode::Window: return "window"; break;
	case EventCode::BarconfigUpdate: return "barconfig_update"; break;
	case EventCode::Binding: return "binding"; break;
	case EventCode::Shutdown: return "shutdown"; break;
	case EventCode::Tick: return "tick"; break;
	case EventCode::BarStateUpdate: return "bar_state_update"; break;
	case EventCode::Input: return "input"; break;

	case EventCode::Unknown: return "unknown"; break;
	}
}

} // namespace qs::i3::ipc
