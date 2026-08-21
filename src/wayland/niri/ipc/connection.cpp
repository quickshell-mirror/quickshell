#include "connection.hpp"
#include <algorithm>
#include <functional>
#include <utility>

#include <qcontainerfwd.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/logcat.hpp"
#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "cast.hpp"
#include "monitor.hpp"
#include "window.hpp"
#include "workspace.hpp"
namespace qs::niri::ipc {

namespace {
QS_LOGGING_CATEGORY(logNiriIpc, "quickshell.niri.ipc", QtWarningMsg);
QS_LOGGING_CATEGORY(logNiriIpcEvents, "quickshell.niri.ipc.events", QtWarningMsg);

/// Write one JSON request line to a niri socket.
void writeRequestLine(QLocalSocket* socket, const QByteArray& json) {
	socket->write(json + '\n');
	socket->flush();
}
} // namespace

NiriIpc::NiriIpc() {
	auto socketPath = qEnvironmentVariable("NIRI_SOCKET");
	if (socketPath.isEmpty()) {
		qCWarning(logNiriIpc) << "$NIRI_SOCKET is unset. Cannot connect to niri.";
		return;
	}

	this->mSocketPath = socketPath;

	// clang-format off
	QObject::connect(&this->eventSocket, &QLocalSocket::errorOccurred, this, &NiriIpc::socketError);
	QObject::connect(&this->eventSocket, &QLocalSocket::stateChanged, this, &NiriIpc::socketStateChanged);
	QObject::connect(&this->eventSocket, &QLocalSocket::readyRead, this, &NiriIpc::socketReady);
	QObject::connect(&this->eventSocket, &QLocalSocket::connected, this, [this]() {
		this->eventReader.setDevice(&this->eventSocket);
		writeRequestLine(&this->eventSocket, "\"EventStream\"");
	});

	this->reconnectTimer.setSingleShot(true);
	this->reconnectTimer.setInterval(2000);
	QObject::connect(&this->reconnectTimer, &QTimer::timeout, this, [this]() {
		qCInfo(logNiriIpc) << "Attempting to reconnect to niri event socket.";
		this->eventSocket.connectToServer(this->mSocketPath, QLocalSocket::ReadWrite);
	});
	// clang-format on

	this->makeRequest("\"Version\"", [&, this](bool success, const QByteArray& resp) {
		if (!success) {
			qCWarning(logNiriIpc) << "Niri ipc status request failed.";
		}

		qCDebug(logNiriIpc) << "Niri version response:" << success << resp;

		this->eventSocket.connectToServer(this->mSocketPath, QLocalSocket::ReadWrite);
		// Niri sends no output events, so monitors must be queried explicitly.
		this->refreshMonitors();
		// Workspaces are delivered by the event stream's initial WorkspacesChanged.
	});
}

QString NiriIpc::socketPath() const { return this->mSocketPath; }

void NiriIpc::socketError(QLocalSocket::LocalSocketError error) const {
	if (!this->valid) {
		qCWarning(logNiriIpc) << "Unable to connect to niri socket:" << error;
	} else {
		qCWarning(logNiriIpc) << "Niri socket error:" << error;
	}
}

void NiriIpc::socketStateChanged(QLocalSocket::LocalSocketState state) {
	if (state == QLocalSocket::ConnectedState) {
		this->reconnectTimer.stop();
		qCInfo(logNiriIpc) << "Niri event socket connected.";
		emit this->connected();
	} else if (state == QLocalSocket::UnconnectedState && this->valid) {
		qCWarning(logNiriIpc) << "Niri event socket disconnected. Will retry.";
		this->valid = false;
		this->reconnectTimer.start();
		return;
	}

	this->valid = state == QLocalSocket::ConnectedState;
}

void NiriIpc::socketReady() {
	while (true) {
		this->eventReader.startTransaction();
		auto rawEvent = this->eventReader.readUntil('\n');
		if (!this->eventReader.commitTransaction()) return;

		auto json = QJsonDocument::fromJson(rawEvent);
		if (!json.isObject()) {
			qCWarning(logNiriIpc) << "Failed to parse niri event:" << rawEvent;
			continue;
		}

		auto object = json.object();

		// The EventStream reply line, not an event.
		if (object.contains("Ok")) continue;
		if (object.contains("Err")) {
			qCWarning(logNiriIpc) << "Niri event stream error:" << object.value("Err").toString();
			continue;
		}

		// An event object has exactly one key: the event name.
		if (object.isEmpty()) continue;
		auto entry = object.constBegin();
		auto name = entry.key().toUtf8();
		auto data = QJsonDocument(entry.value().toObject()).toJson(QJsonDocument::Compact);

		qCDebug(logNiriIpcEvents) << "Received event:" << name << data;

		this->event.name = name;
		this->event.data = data;
		this->onEvent(&this->event);
		emit this->rawEvent(&this->event);
	}
}

void NiriIpc::makeRequest(
    const QByteArray& request,
    const std::function<void(bool, QByteArray)>& callback
) {
	auto* requestSocket = new QLocalSocket(this);
	qCDebug(logNiriIpc) << "Making request:" << request;

	auto connectedCallback = [this, request, requestSocket, callback]() {
		auto responseCallback = [requestSocket, callback]() {
			if (!requestSocket->canReadLine()) return;

			auto response = requestSocket->readLine();
			response.chop(1); // trailing newline
			callback(true, std::move(response));
			requestSocket->deleteLater();
		};

		QObject::connect(requestSocket, &QLocalSocket::readyRead, this, responseCallback);

		writeRequestLine(requestSocket, request);
	};

	auto errorCallback = [=](QLocalSocket::LocalSocketError error) {
		qCWarning(logNiriIpc) << "Error making request:" << error << "request:" << request;
		requestSocket->deleteLater();
		callback(false, {});
	};

	QObject::connect(requestSocket, &QLocalSocket::connected, this, connectedCallback);
	QObject::connect(requestSocket, &QLocalSocket::errorOccurred, this, errorCallback);

	requestSocket->connectToServer(this->mSocketPath);
}

void NiriIpc::dispatch(const QString& request) {
	this->makeRequest(
	    "{\"Action\":" + request.toUtf8() + "}",
	    [request](bool success, const QByteArray& response) {
		    if (!success) {
			    qCWarning(logNiriIpc) << "Failed to request dispatch of" << request;
			    return;
		    }

		    auto json = QJsonDocument::fromJson(response).object();
		    if (json.contains("Err")) {
			    qCWarning(logNiriIpc) << "Dispatch request" << request << "failed with error"
			                          << json.value("Err").toString();
		    }
	    }
	);
}

ObjectModel<NiriMonitor>* NiriIpc::monitors() { return &this->mMonitors; }

ObjectModel<NiriWorkspace>* NiriIpc::workspaces() { return &this->mWorkspaces; }

ObjectModel<NiriWindow>* NiriIpc::windows() { return &this->mWindows; }

ObjectModel<NiriCast>* NiriIpc::casts() { return &this->mCasts; }

QVariantMap NiriIpcEvent::parse() const {
	return QJsonDocument::fromJson(this->data).object().toVariantMap();
}

QString NiriIpcEvent::nameStr() const { return QString::fromUtf8(this->name); }
QString NiriIpcEvent::dataStr() const { return QString::fromUtf8(this->data); }

NiriIpc* NiriIpc::instance() {
	static NiriIpc* instance = nullptr;

	if (instance == nullptr) {
		instance = new NiriIpc();
	}

	return instance;
}

void NiriIpc::onEvent(NiriIpcEvent* event) {
	if (event->name == "WorkspacesChanged") {
		this->updateWorkspaces(event->parse().value("workspaces").toList());
	} else if (event->name == "WorkspaceActivated") {
		auto data = event->parse();
		auto id = data.value("id").value<qint64>();
		auto* workspace = this->findWorkspaceById(id);

		if (!workspace) {
			qCWarning(logNiriIpc) << "WorkspaceActivated for unknown workspace id" << id;
			return;
		}

		auto* monitor = workspace->bindableMonitor().value();

		// All other workspaces on the same output become inactive.
		for (auto* other: this->mWorkspaces.valueList()) {
			if (other->bindableMonitor().value() == monitor) {
				other->bindableActive().setValue(other == workspace);
			}
		}

		if (monitor) monitor->setActiveWorkspace(workspace);

		if (data.value("focused").toBool()) {
			this->bFocusedWorkspace = workspace;
			this->setFocusedMonitor(monitor);
		}
	} else if (event->name == "WorkspaceUrgencyChanged") {
		auto data = event->parse();
		auto id = data.value("id").value<qint64>();
		auto* workspace = this->findWorkspaceById(id);

		if (!workspace) {
			qCWarning(logNiriIpc) << "WorkspaceUrgencyChanged for unknown workspace id" << id;
			return;
		}

		workspace->bindableUrgent().setValue(data.value("urgent").toBool());
	} else if (event->name == "WorkspaceActiveWindowChanged") {
		auto data = event->parse();
		auto id = data.value("workspace_id").value<qint64>();
		auto* workspace = this->findWorkspaceById(id);

		if (!workspace) {
			qCWarning(logNiriIpc) << "WorkspaceActiveWindowChanged for unknown workspace id" << id;
			return;
		}

		workspace->bindableActiveWindowId().setValue(data.value("active_window_id").value<qint64>());
	} else if (event->name == "WindowsChanged") {
		this->updateWindows(event->parse().value("windows").toList());
	} else if (event->name == "WindowOpenedOrChanged") {
		auto window = event->parse().value("window").toMap();
		auto id = window.value("id").value<qint64>();

		auto* existing = this->findWindowById(id);
		auto* windowObj = existing;

		if (!windowObj) {
			windowObj = new NiriWindow(this);
		}

		windowObj->updateFromObject(window);

		if (!existing) {
			this->mWindows.insertObject(windowObj);
		}

		// If this window is focused, all other windows are no longer focused.
		if (windowObj->bindableFocused().value()) {
			this->setActiveWindow(windowObj);
		}
	} else if (event->name == "WindowClosed") {
		auto id = event->parse().value("id").value<qint64>();
		auto* window = this->findWindowById(id);

		if (!window) {
			qCWarning(logNiriIpc) << "WindowClosed for unknown window id" << id;
			return;
		}

		if (this->bActiveWindow == window) {
			this->bActiveWindow = nullptr;
		}

		this->mWindows.removeObject(window);
		window->deleteLater();
	} else if (event->name == "WindowFocusChanged") {
		auto id = event->parse().value("id").value<qint64>();

		if (id == 0) {
			this->setActiveWindow(nullptr);
		} else {
			auto* window = this->findWindowById(id);

			if (!window) {
				qCWarning(logNiriIpc) << "WindowFocusChanged for unknown window id" << id;
				return;
			}

			this->setActiveWindow(window);
		}
	} else if (event->name == "WindowUrgencyChanged") {
		auto data = event->parse();
		auto id = data.value("id").value<qint64>();
		auto* window = this->findWindowById(id);

		if (!window) {
			qCWarning(logNiriIpc) << "WindowUrgencyChanged for unknown window id" << id;
			return;
		}

		window->bindableUrgent().setValue(data.value("urgent").toBool());
	} else if (event->name == "WindowLayoutsChanged") {
		auto changes = event->parse().value("changes").toList();

		for (const auto& change: changes) {
			// Each change is an (id, layout) pair.
			auto pair = change.toList();
			if (pair.size() != 2) continue;

			auto* window = this->findWindowById(pair[0].value<qint64>());
			if (!window) continue;

			window->setLayout(pair[1].toMap());
		}
	} else if (event->name == "WindowFocusTimestampChanged") {
		auto data = event->parse();
		auto id = data.value("id").value<qint64>();
		auto* window = this->findWindowById(id);

		if (!window) {
			qCWarning(logNiriIpc) << "WindowFocusTimestampChanged for unknown window id" << id;
			return;
		}

		window->setFocusTimestamp(data.value("focus_timestamp").toMap());
	} else if (event->name == "OverviewOpenedOrClosed") {
		this->bOverviewOpen = event->parse().value("is_open").toBool();
	} else if (event->name == "KeyboardLayoutsChanged") {
		auto layouts = event->parse().value("keyboard_layouts").toMap();

		auto names = QStringList();
		for (const auto& name: layouts.value("names").toList()) {
			names.push_back(name.toString());
		}

		this->mKeyboardLayouts = names;
		emit this->keyboardLayoutsChanged();
		this->bCurrentKeyboardLayout = layouts.value("current_idx").value<qint32>();
	} else if (event->name == "KeyboardLayoutSwitched") {
		this->bCurrentKeyboardLayout = event->parse().value("idx").value<qint32>();
	} else if (event->name == "ConfigLoaded") {
		emit this->configLoaded(event->parse().value("failed").toBool());
	} else if (event->name == "ScreenshotCaptured") {
		emit this->screenshotCaptured(event->parse().value("path").toString());
	} else if (event->name == "CastsChanged") {
		this->updateCasts(event->parse().value("casts").toList());
	} else if (event->name == "CastStartedOrChanged") {
		auto cast = event->parse().value("cast").toMap();
		auto streamId = cast.value("stream_id").value<qint64>();

		auto* existing = this->findCastByStreamId(streamId);
		auto* castObj = existing;

		if (!castObj) {
			castObj = new NiriCast(this);
		}

		castObj->updateFromObject(cast);

		if (!existing) {
			this->mCasts.insertObject(castObj);
		}
	} else if (event->name == "CastStopped") {
		auto streamId = event->parse().value("stream_id").value<qint64>();
		auto* cast = this->findCastByStreamId(streamId);

		if (!cast) {
			qCWarning(logNiriIpc) << "CastStopped for unknown stream id" << streamId;
			return;
		}

		this->mCasts.removeObject(cast);
		cast->deleteLater();
	} else {
		qCDebug(logNiriIpcEvents) << "Unhandled niri event:" << event->name;
	}
}

NiriWorkspace* NiriIpc::findWorkspaceById(qint64 id) {
	const auto& mList = this->mWorkspaces.valueList();

	auto workspaceIter =
	    std::ranges::find_if(mList, [id](NiriWorkspace* m) { return m->bindableId().value() == id; });

	return workspaceIter == mList.end() ? nullptr : *workspaceIter;
}

NiriWindow* NiriIpc::findWindowById(qint64 id) {
	const auto& mList = this->mWindows.valueList();

	auto windowIter =
	    std::ranges::find_if(mList, [id](NiriWindow* m) { return m->bindableId().value() == id; });

	return windowIter == mList.end() ? nullptr : *windowIter;
}

NiriCast* NiriIpc::findCastByStreamId(qint64 streamId) {
	const auto& mList = this->mCasts.valueList();

	auto castIter = std::ranges::find_if(mList, [streamId](NiriCast* m) {
		return m->bindableStreamId().value() == streamId;
	});

	return castIter == mList.end() ? nullptr : *castIter;
}

void NiriIpc::updateWorkspaces(const QVariantList& workspaces) {
	// Clear monitors' active workspace links before destroying workspace objects.
	for (auto* monitor: this->mMonitors.valueList()) {
		monitor->setActiveWorkspace(nullptr);
	}

	this->bFocusedWorkspace = nullptr;
	this->setFocusedMonitor(nullptr);

	// WorkspacesChanged is a full replace — clear and rebuild so the model is
	// always correctly sorted even when existing workspaces change idx or output.
	for (auto* workspace: this->mWorkspaces.valueList()) {
		this->mWorkspaces.removeObject(workspace);
		workspace->deleteLater();
	}

	NiriWorkspace* focused = nullptr;

	for (const auto& entry: workspaces) {
		auto object = entry.toMap();
		auto* workspace = new NiriWorkspace(this);
		workspace->updateFromObject(object);
		this->mWorkspaces.insertObjectSorted(workspace, &NiriIpc::compareWorkspaces);

		if (object.value("is_focused").toBool()) {
			focused = workspace;
		}
	}

	this->bFocusedWorkspace = focused;
	this->setFocusedMonitor(focused ? focused->bindableMonitor().value() : nullptr);

	// Relink the active workspace of every monitor.
	for (auto* monitor: this->mMonitors.valueList()) {
		NiriWorkspace* active = nullptr;

		for (auto* workspace: this->mWorkspaces.valueList()) {
			if (workspace->bindableActive().value() && workspace->bindableMonitor().value() == monitor) {
				active = workspace;
				break;
			}
		}

		monitor->setActiveWorkspace(active);
	}
}

void NiriIpc::refreshWorkspaces() {
	if (this->requestingWorkspaces) return;
	this->requestingWorkspaces = true;

	this->makeRequest("\"Workspaces\"", [this](bool success, const QByteArray& resp) {
		this->requestingWorkspaces = false;
		if (!success) return;

		qCDebug(logNiriIpc) << "Parsing workspaces response";
		auto reply = QJsonDocument::fromJson(resp).object();
		auto workspaces = reply.value("Ok").toObject().value("Workspaces").toArray();
		this->updateWorkspaces(workspaces.toVariantList());
	});
}

void NiriIpc::updateWindows(const QVariantList& windows) {
	const auto& mList = this->mWindows.valueList();
	auto ids = QVector<qint64>();
	NiriWindow* focused = nullptr;

	for (const auto& entry: windows) {
		auto object = entry.toMap();
		auto id = object.value("id").value<qint64>();

		auto* window = this->findWindowById(id);
		auto existed = window != nullptr;

		if (!existed) {
			window = new NiriWindow(this);
		}

		window->updateFromObject(object);

		if (!existed) {
			this->mWindows.insertObject(window);
		}

		if (object.value("is_focused").toBool()) {
			focused = window;
		}

		ids.push_back(id);
	}

	// Windows missing from the full state were closed.
	auto removedWindows = QVector<NiriWindow*>();

	for (auto* window: mList) {
		if (!ids.contains(window->bindableId().value())) {
			removedWindows.push_back(window);
		}
	}

	if (removedWindows.contains(this->bActiveWindow.value())) {
		this->bActiveWindow = nullptr;
	}

	for (auto* window: removedWindows) {
		this->mWindows.removeObject(window);
		window->deleteLater();
	}

	this->bActiveWindow = focused;
}

void NiriIpc::updateCasts(const QVariantList& casts) {
	const auto& mList = this->mCasts.valueList();
	auto streamIds = QVector<qint64>();

	for (const auto& entry: casts) {
		auto object = entry.toMap();
		auto streamId = object.value("stream_id").value<qint64>();

		auto* cast = this->findCastByStreamId(streamId);
		auto existed = cast != nullptr;

		if (!existed) {
			cast = new NiriCast(this);
		}

		cast->updateFromObject(object);

		if (!existed) {
			this->mCasts.insertObject(cast);
		}

		streamIds.push_back(streamId);
	}

	// Casts missing from the full state were stopped.
	auto removedCasts = QVector<NiriCast*>();

	for (auto* cast: mList) {
		if (!streamIds.contains(cast->bindableStreamId().value())) {
			removedCasts.push_back(cast);
		}
	}

	for (auto* cast: removedCasts) {
		this->mCasts.removeObject(cast);
		cast->deleteLater();
	}
}

void NiriIpc::refreshCasts() {
	if (this->requestingCasts) return;
	this->requestingCasts = true;

	this->makeRequest("\"Casts\"", [this](bool success, const QByteArray& resp) {
		this->requestingCasts = false;
		if (!success) return;

		qCDebug(logNiriIpc) << "Parsing casts response";
		auto reply = QJsonDocument::fromJson(resp).object();
		auto casts = reply.value("Ok").toObject().value("Casts").toArray();
		this->updateCasts(casts.toVariantList());
	});
}

void NiriIpc::refreshWindows() {
	if (this->requestingWindows) return;
	this->requestingWindows = true;

	this->makeRequest("\"Windows\"", [this](bool success, const QByteArray& resp) {
		this->requestingWindows = false;
		if (!success) return;

		qCDebug(logNiriIpc) << "Parsing windows response";
		auto reply = QJsonDocument::fromJson(resp).object();
		auto windows = reply.value("Ok").toObject().value("Windows").toArray();
		this->updateWindows(windows.toVariantList());
	});
}

NiriMonitor* NiriIpc::findMonitorByName(const QString& name, bool createIfMissing) {
	const auto& mList = this->mMonitors.valueList();

	auto monitorIter = std::ranges::find_if(mList, [name](NiriMonitor* m) {
		return m->bindableName().value() == name;
	});

	if (monitorIter != mList.end()) {
		return *monitorIter;
	} else if (createIfMissing) {
		qCDebug(logNiriIpc) << "Monitor" << name << "requested before creation, performing early init";
		auto* monitor = new NiriMonitor(this);
		monitor->updateInitial(name);
		this->mMonitors.insertObject(monitor);
		return monitor;
	} else {
		return nullptr;
	}
}

NiriMonitor* NiriIpc::monitorFor(QuickshellScreenInfo* screen) {
	// Niri monitors are created by refreshMonitors, which runs at startup, so a monitor
	// will usually already exist. If this call creates the NiriIpc singleton before the
	// first refresh returns, preemptively create one.
	if (screen == nullptr) return nullptr;
	return this->findMonitorByName(screen->name(), !this->monitorsRequested);
}

void NiriIpc::setFocusedMonitor(NiriMonitor* monitor) {
	auto* oldMonitor = this->bFocusedMonitor.value();
	if (monitor == oldMonitor) return;

	if (this->bFocusedMonitor != nullptr) {
		QObject::disconnect(this->bFocusedMonitor, nullptr, this, nullptr);
	}

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &NiriIpc::onFocusedMonitorDestroyed);
	}

	this->bFocusedMonitor = monitor;
}

void NiriIpc::onFocusedMonitorDestroyed() { this->bFocusedMonitor = nullptr; }

void NiriIpc::setActiveWindow(NiriWindow* window) {
	// At most one window is focused at a time.
	for (auto* other: this->mWindows.valueList()) {
		other->bindableFocused().setValue(other == window);
	}

	this->bActiveWindow = window;
}

void NiriIpc::refreshMonitors() {
	if (this->requestingMonitors) return;
	this->requestingMonitors = true;

	this->makeRequest("\"Outputs\"", [this](bool success, const QByteArray& resp) {
		this->requestingMonitors = false;
		if (!success) return;

		this->monitorsRequested = true;

		qCDebug(logNiriIpc) << "Parsing outputs response";
		auto reply = QJsonDocument::fromJson(resp).object();
		auto outputs = reply.value("Ok").toObject().value("Outputs").toObject();

		auto monitors = QVariantList();
		for (const auto& output: outputs) {
			monitors.push_back(output.toObject().toVariantMap());
		}

		this->updateMonitors(monitors);
	});
}

void NiriIpc::updateMonitors(const QVariantList& monitors) {
	const auto& mList = this->mMonitors.valueList();
	auto names = QVector<QString>();

	for (const auto& entry: monitors) {
		auto object = entry.toMap();
		auto name = object.value("name").toString();

		auto* monitor = this->findMonitorByName(name, true);
		monitor->updateFromObject(object);

		names.push_back(name);
	}

	// Monitors missing from the full state were disconnected.
	auto removedMonitors = QVector<NiriMonitor*>();

	for (auto* monitor: mList) {
		if (!names.contains(monitor->bindableName().value())) {
			removedMonitors.push_back(monitor);
		}
	}

	for (auto* monitor: removedMonitors) {
		this->mMonitors.removeObject(monitor);
		monitor->deleteLater();
	}
}

bool NiriIpc::compareWorkspaces(NiriWorkspace* a, NiriWorkspace* b) {
	auto* am = a->bindableMonitor().value();
	auto* bm = b->bindableMonitor().value();
	auto ax = am ? am->bindableX().value() : 0;
	auto bx = bm ? bm->bindableX().value() : 0;

	if (ax != bx) return ax > bx;
	return a->bindableIdx().value() > b->bindableIdx().value();
}

} // namespace qs::niri::ipc
