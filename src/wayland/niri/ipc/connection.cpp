#include "connection.hpp"
#include <algorithm>
#include <functional>

#include <qcontainerfwd.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qprocess.h>
#include <qproperty.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/logcat.hpp"
#include "../../../core/model.hpp"
#include "output.hpp"
#include "window.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

namespace {

QS_LOGGING_CATEGORY(logNiriIpc, "quickshell.niri.ipc", QtWarningMsg);
QS_LOGGING_CATEGORY(logNiriIpcEvents, "quickshell.niri.ipc.events", QtWarningMsg);

// diffUpdate inserts without removing the prior row; reorder after sort avoids duplicate model rows.
void reorderNiriWorkspaceModel(ObjectModel<NiriWorkspace>& model, const QList<NiriWorkspace*>& sorted) {
	QList<NiriWorkspace*> order;
	order.reserve(sorted.size());
	for (auto* w: sorted) {
		if (!order.contains(w)) order.append(w);
	}

	auto& list = model.valueList();
	for (qsizetype i = 0; i < list.length();) {
		auto* o = list.at(i);
		if (!order.contains(o) || list.indexOf(o) != i) model.removeAt(i);
		else i++;
	}

	for (qsizetype t = 0; t < order.length(); ++t) {
		auto* w = order.at(t);
		auto c = list.indexOf(w);
		if (c < 0 || c == t) continue;
		model.removeAt(c);
		model.insertObject(w, t);
	}
}

} // namespace

void NiriIpcEvent::set(const QString& type, const QJsonObject& data) {
	this->mType = type;
	this->mData = data;
}

void NiriIpcEvent::reset() {
	this->mType.clear();
	this->mData = QJsonObject();
}

NiriIpc::NiriIpc() {
	this->mSocketPath = qEnvironmentVariable("NIRI_SOCKET");
	if (this->mSocketPath.isEmpty()) {
		qWarning() << "$NIRI_SOCKET is unset. Cannot connect to niri.";
		return;
	}

	// clang-format off
	QObject::connect(&this->eventSocket, &QLocalSocket::errorOccurred, this, &NiriIpc::eventSocketError);
	QObject::connect(&this->eventSocket, &QLocalSocket::stateChanged, this, &NiriIpc::eventSocketStateChanged);
	QObject::connect(&this->eventSocket, &QLocalSocket::connected, this, &NiriIpc::eventSocketConnected);
	QObject::connect(&this->eventSocket, &QLocalSocket::readyRead, this, &NiriIpc::eventSocketReady);
	// clang-format on

	this->eventSocket.connectToServer(this->mSocketPath, QLocalSocket::ReadWrite);
}

QString NiriIpc::socketPath() const { return this->mSocketPath; }

void NiriIpc::eventSocketError(QLocalSocket::LocalSocketError error) const {
	if (!this->valid) {
		qWarning() << "Unable to connect to niri event socket:" << error;
	} else {
		qWarning() << "Niri event socket error:" << error;
	}
}

void NiriIpc::eventSocketStateChanged(QLocalSocket::LocalSocketState state) {
	if (state == QLocalSocket::UnconnectedState && this->valid) {
		qCWarning(logNiriIpc) << "Niri event socket disconnected.";
	}

	this->valid = state == QLocalSocket::ConnectedState;
}

void NiriIpc::eventSocketConnected() {
	qCInfo(logNiriIpc) << "Niri event socket connected.";

	// Send EventStream request to start receiving events
	this->eventSocket.write("\"EventStream\"\n");
	this->eventSocket.flush();

	// Fetch initial data
	this->refreshOutputs();
	this->refreshWorkspaces();
	this->refreshWindows();

	emit this->connected();
}

void NiriIpc::eventSocketReady() {
	while (this->eventSocket.canReadLine()) {
		auto rawLine = this->eventSocket.readLine();
		if (rawLine.isEmpty()) break;

		// Remove trailing newline
		rawLine = rawLine.trimmed();
		if (rawLine.isEmpty()) continue;

		qCDebug(logNiriIpcEvents) << "Received event:" << rawLine;

		auto doc = QJsonDocument::fromJson(rawLine);
		if (!doc.isObject()) {
			qCDebug(logNiriIpcEvents) << "Ignoring non-object event line:" << rawLine;
			continue;
		}

		this->onEvent(doc.object());
	}
}

void NiriIpc::onEvent(const QJsonObject& eventObject) {
	// Each niri event is a JSON object with a single key being the event type
	auto keys = eventObject.keys();
	if (keys.isEmpty()) return;

	auto eventType = keys.first();
	auto eventData = eventObject.value(eventType).toObject();

	// Dispatch to specific handlers first so models are updated
	if (eventType == "WorkspacesChanged") {
		this->handleWorkspacesChanged(eventData);
	} else if (eventType == "WindowOpenedOrChanged") {
		this->handleWindowOpenedOrChanged(eventData);
	} else if (eventType == "WindowClosed") {
		this->handleWindowClosed(eventData);
	} else if (eventType == "WindowsChanged") {
		this->handleWindowsChanged(eventData);
	} else if (eventType == "WorkspaceActivated") {
		this->handleWorkspaceActivated(eventData);
	} else if (eventType == "WorkspaceActiveWindowChanged") {
		this->handleWorkspaceActiveWindowChanged(eventData);
	} else if (eventType == "WindowFocusChanged") {
		this->handleWindowFocusChanged(eventData);
	} else if (eventType == "WindowLayoutsChanged") {
		this->handleWindowLayoutsChanged(eventData);
	} else if (eventType == "OverviewOpenedOrClosed") {
		this->handleOverviewOpenedOrClosed(eventData);
	} else if (eventType == "OutputsChanged") {
		this->refreshOutputs();
	} else if (eventType == "ConfigLoaded") {
		this->refreshOutputs();
	} else if (eventType == "KeyboardLayoutsChanged") {
		this->handleKeyboardLayoutsChanged(eventData);
	} else if (eventType == "KeyboardLayoutSwitched") {
		this->handleKeyboardLayoutSwitched(eventData);
	}

	// Emit raw event AFTER handlers so QML consumers see updated state
	this->event.set(eventType, eventData);
	emit this->rawEvent(&this->event);
	this->event.reset();
}

void NiriIpc::handleWorkspacesChanged(const QJsonObject& data) {
	auto workspacesArray = data.value("workspaces").toArray();
	this->updateWorkspacesFromArray(workspacesArray);
}

void NiriIpc::handleWorkspaceActivated(const QJsonObject& data) {
	auto workspaceId = static_cast<qint64>(data.value("id").toDouble(-1));
	auto isFocused = data.value("focused").toBool();

	auto* activated = this->findWorkspaceById(workspaceId);
	if (!activated) {
		// Unknown workspace — same idea as Hyprland falling back to j/workspaces
		this->refreshWorkspaces();
		return;
	}

	auto targetOutput = activated->bindableOutput().value();

	Qt::beginPropertyUpdateGroup();
	for (auto* ws: this->mWorkspaces.valueList()) {
		// Clear active on same output
		if (ws->bindableOutput().value() == targetOutput && ws != activated) {
			ws->setActive(false);
		}
		// If the activated workspace is focused, clear focus on all others
		if (isFocused && ws != activated) {
			ws->setFocused(false);
		}
	}
	activated->setActive(true);
	if (isFocused) {
		activated->setFocused(true);
	}
	Qt::endPropertyUpdateGroup();

	emit this->workspacesUpdated();

	// Event-stream ordering can leave is_focused stale on toplevels (e.g. empty workspace:
	// no WindowFocusChanged before the shell reads state). Request::Windows is authoritative;
	// see https://docs.rs/niri-ipc/latest/niri_ipc/
	if (isFocused) {
		this->refreshWindows();
	}
}

void NiriIpc::handleWorkspaceActiveWindowChanged(const QJsonObject& data) {
	auto workspaceId = static_cast<qint64>(data.value("workspace_id").toDouble(-1));
	auto activeWindowIdValue = data.value("active_window_id");
	auto activeWindowId = activeWindowIdValue.isNull() || activeWindowIdValue.isUndefined()
	    ? static_cast<qint64>(-1)
	    : static_cast<qint64>(activeWindowIdValue.toDouble(-1));

	auto* workspace = this->findWorkspaceById(workspaceId);
	if (workspace) {
		workspace->setActiveWindowId(activeWindowId);
		emit this->workspacesUpdated();
	}
}

void NiriIpc::handleWindowOpenedOrChanged(const QJsonObject& data) {
	auto windowObj = data.value("window").toObject();
	auto windowId = static_cast<qint64>(windowObj.value("id").toDouble(-1));
	if (windowId < 0) return;

	auto workspaceId = static_cast<qint64>(windowObj.value("workspace_id").toDouble(-1));
	auto outputName = this->resolveWindowOutput(workspaceId);

	auto* existing = this->findWindowById(windowId);
	if (existing) {
		// Update existing window
		existing->updateFromJson(windowObj, outputName);

		if (existing->bindableFocused().value()) {
			// Clear focus on previously focused window
			auto* oldFocused = this->bFocusedWindow.value();
			if (oldFocused && oldFocused != existing) {
				oldFocused->setFocused(false);
			}
			this->bFocusedWindow = existing;
		}
	} else {
		// Create new window
		auto* window = new NiriWindow(this);
		window->updateFromJson(windowObj, outputName);
		this->mWindows.insertObject(window);

		if (window->bindableFocused().value()) {
			auto* oldFocused = this->bFocusedWindow.value();
			if (oldFocused) {
				oldFocused->setFocused(false);
			}
			this->bFocusedWindow = window;
		}

		qCDebug(logNiriIpc) << "New window created with id" << windowId;
	}

	emit this->windowsUpdated();
}

void NiriIpc::handleWindowClosed(const QJsonObject& data) {
	auto windowId = static_cast<qint64>(data.value("id").toDouble(-1));
	if (windowId < 0) return;

	auto* window = this->findWindowById(windowId);
	if (!window) {
		qCWarning(logNiriIpc) << "Got WindowClosed for id" << windowId
		                      << "which was not previously tracked.";
		return;
	}

	if (this->bFocusedWindow.value() == window) {
		this->bFocusedWindow = nullptr;
	}

	this->mWindows.removeObject(window);
	window->deleteLater();

	qCDebug(logNiriIpc) << "Window closed with id" << windowId;
	emit this->windowsUpdated();
}

void NiriIpc::handleWindowsChanged(const QJsonObject& data) {
	auto windowsArray = data.value("windows").toArray();
	this->updateWindowsFromArray(windowsArray);
}

void NiriIpc::handleWindowFocusChanged(const QJsonObject& data) {
	auto focusedIdValue = data.value("id");

	// Clear old focus
	auto* oldFocused = this->bFocusedWindow.value();
	if (oldFocused) {
		oldFocused->setFocused(false);
	}

	if (focusedIdValue.isNull() || focusedIdValue.isUndefined()) {
		this->bFocusedWindow = nullptr;
		return;
	}

	auto focusedId = static_cast<qint64>(focusedIdValue.toDouble(-1));
	auto* window = this->findWindowById(focusedId);
	if (window) {
		window->setFocused(true);
		this->bFocusedWindow = window;
	} else {
		this->bFocusedWindow = nullptr;
	}

	emit this->windowsUpdated();
}

void NiriIpc::handleWindowLayoutsChanged(const QJsonObject& data) {
	auto changes = data.value("changes").toArray();

	for (const auto& change: changes) {
		auto changeArray = change.toArray();
		if (changeArray.size() < 2) continue;

		auto windowId = static_cast<qint64>(changeArray.at(0).toDouble(-1));
		auto layout = changeArray.at(1).toObject();

		auto* window = this->findWindowById(windowId);
		if (window) {
			window->updateLayout(layout);
		}
	}

	emit this->windowsUpdated();
}

void NiriIpc::handleOverviewOpenedOrClosed(const QJsonObject& data) {
	this->bOverviewActive = data.value("is_open").toBool();
	qCDebug(logNiriIpc) << "Overview" << (this->bOverviewActive.value() ? "opened" : "closed");
}

void NiriIpc::handleKeyboardLayoutsChanged(const QJsonObject& data) {
	auto layoutsObj = data.value("keyboard_layouts").toObject();
	auto names = layoutsObj.value("names").toArray();
	auto currentIdx = layoutsObj.value("current_idx").toInt();

	QStringList layoutNames;
	for (const auto& name: names) {
		layoutNames.append(name.toString());
	}

	Qt::beginPropertyUpdateGroup();
	this->bKeyboardLayoutNames = layoutNames;
	this->bCurrentKeyboardLayoutIndex = currentIdx;
	Qt::endPropertyUpdateGroup();

	qCDebug(logNiriIpc) << "Keyboard layouts changed:" << layoutNames;
}

void NiriIpc::handleKeyboardLayoutSwitched(const QJsonObject& data) {
	this->bCurrentKeyboardLayoutIndex = data.value("idx").toInt();
	qCDebug(logNiriIpc) << "Keyboard layout switched to index"
	                    << this->bCurrentKeyboardLayoutIndex.value();
}

void NiriIpc::makeRequest(
    const QByteArray& request,
    const std::function<void(bool, const QJsonObject&)>& callback
) {
	auto* requestSocket = new QLocalSocket(this);
	auto* buffer = new QByteArray();

	qCDebug(logNiriIpc) << "Making request:" << request.trimmed();

	auto connectedCallback = [request, requestSocket]() {
		requestSocket->write(request);
		requestSocket->flush();
	};

	auto readyCallback = [requestSocket, buffer, callback]() {
		buffer->append(requestSocket->readAll());
		auto newlineIdx = buffer->indexOf('\n');
		if (newlineIdx < 0) return; // Wait for more data

		auto responseLine = buffer->left(newlineIdx);
		auto doc = QJsonDocument::fromJson(responseLine);

		if (doc.isObject()) {
			callback(true, doc.object());
		} else {
			qCWarning(logNiriIpc) << "Invalid JSON response:" << responseLine;
			callback(false, {});
		}

		requestSocket->disconnect();
		requestSocket->disconnectFromServer();
		requestSocket->deleteLater();
		delete buffer;
	};

	auto errorCallback = [requestSocket, buffer, callback, request](
	                          QLocalSocket::LocalSocketError error
	                      ) {
		qCWarning(logNiriIpc) << "Error making request:" << error << "request:" << request.trimmed();
		requestSocket->disconnect();
		requestSocket->deleteLater();
		delete buffer;
		callback(false, {});
	};

	QObject::connect(requestSocket, &QLocalSocket::connected, this, connectedCallback);
	QObject::connect(requestSocket, &QLocalSocket::readyRead, this, readyCallback);
	QObject::connect(requestSocket, &QLocalSocket::errorOccurred, this, errorCallback);

	requestSocket->connectToServer(this->mSocketPath);
}

void NiriIpc::dispatch(const QStringList& args) {
	QStringList command = {"niri", "msg", "action"};
	command.append(args);

	qCDebug(logNiriIpc) << "Dispatching:" << command.join(" ");
	QProcess::startDetached(command.first(), command.mid(1));
}

ObjectModel<NiriOutput>* NiriIpc::outputs() { return &this->mOutputs; }
ObjectModel<NiriWorkspace>* NiriIpc::workspaces() { return &this->mWorkspaces; }
ObjectModel<NiriWindow>* NiriIpc::windows() { return &this->mWindows; }

NiriIpc* NiriIpc::instance() {
	static NiriIpc* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new NiriIpc();
	}

	return instance;
}

NiriOutput* NiriIpc::findOutputByName(const QString& name) {
	const auto& mList = this->mOutputs.valueList();
	auto iter = std::ranges::find_if(mList, [&name](NiriOutput* o) {
		return o->bindableName().value() == name;
	});
	return iter == mList.end() ? nullptr : *iter;
}

NiriWorkspace* NiriIpc::findWorkspaceById(qint64 id) {
	const auto& mList = this->mWorkspaces.valueList();
	auto iter = std::ranges::find_if(mList, [id](NiriWorkspace* ws) {
		return ws->bindableId().value() == id;
	});
	return iter == mList.end() ? nullptr : *iter;
}

NiriWindow* NiriIpc::findWindowById(qint64 id) {
	const auto& mList = this->mWindows.valueList();
	auto iter = std::ranges::find_if(mList, [id](NiriWindow* w) {
		return w->bindableId().value() == id;
	});
	return iter == mList.end() ? nullptr : *iter;
}

QString NiriIpc::resolveWindowOutput(qint64 workspaceId) {
	auto* workspace = this->findWorkspaceById(workspaceId);
	return workspace ? workspace->bindableOutput().value() : QString();
}

void NiriIpc::refreshOutputs() {
	if (this->requestingOutputs) return;
	this->requestingOutputs = true;

	this->makeRequest("\"Outputs\"\n", [this](bool success, const QJsonObject& resp) {
		this->requestingOutputs = false;
		if (!success) return;

		auto ok = resp.value("Ok").toObject();
		if (ok.contains("Outputs")) {
			this->updateOutputsFromObject(ok.value("Outputs").toObject());
		} else if (resp.contains("Err")) {
			qCWarning(logNiriIpc) << "Outputs request failed:" << resp.value("Err").toString();
		}
	});
}

void NiriIpc::refreshWorkspaces() {
	if (this->requestingWorkspaces) return;
	this->requestingWorkspaces = true;

	this->makeRequest("\"Workspaces\"\n", [this](bool success, const QJsonObject& resp) {
		this->requestingWorkspaces = false;
		if (!success) return;

		auto ok = resp.value("Ok").toObject();
		if (ok.contains("Workspaces")) {
			this->updateWorkspacesFromArray(ok.value("Workspaces").toArray());
		} else if (resp.contains("Err")) {
			qCWarning(logNiriIpc) << "Workspaces request failed:" << resp.value("Err").toString();
		}
	});
}

void NiriIpc::refreshWindows() {
	if (this->requestingWindows) return;
	this->requestingWindows = true;

	this->makeRequest("\"Windows\"\n", [this](bool success, const QJsonObject& resp) {
		this->requestingWindows = false;
		if (!success) return;

		auto ok = resp.value("Ok").toObject();
		if (ok.contains("Windows")) {
			this->updateWindowsFromArray(ok.value("Windows").toArray());
		} else if (resp.contains("Err")) {
			qCWarning(logNiriIpc) << "Windows request failed:" << resp.value("Err").toString();
		}
	});
}

void NiriIpc::updateOutputsFromObject(const QJsonObject& outputsObject) {
	qCDebug(logNiriIpc) << "Updating outputs";

	auto names = QVector<QString>();
	const auto& mList = this->mOutputs.valueList();

	for (auto it = outputsObject.begin(); it != outputsObject.end(); ++it) {
		auto outputObj = it.value().toObject();
		auto name = outputObj.value("name").toString();
		if (name.isEmpty()) continue;

		names.push_back(name);

		auto* existing = this->findOutputByName(name);
		if (existing) {
			existing->updateFromJson(outputObj);
		} else {
			auto* output = new NiriOutput(this);
			output->updateFromJson(outputObj);
			this->mOutputs.insertObject(output);
			qCDebug(logNiriIpc) << "New output:" << name;
		}
	}

	// Remove outputs no longer present
	auto removedOutputs = QVector<NiriOutput*>();
	for (auto* output: mList) {
		if (!names.contains(output->bindableName().value())) {
			removedOutputs.push_back(output);
		}
	}

	for (auto* output: removedOutputs) {
		this->mOutputs.removeObject(output);
		output->deleteLater();
	}

	emit this->outputsUpdated();
}

void NiriIpc::updateWorkspacesFromArray(const QJsonArray& workspacesArray) {
	qCDebug(logNiriIpc) << "Updating workspaces";

	auto ids = QVector<qint64>();
	const auto& mList = this->mWorkspaces.valueList();

	for (const auto& entry: workspacesArray) {
		auto wsObj = entry.toObject();
		auto id = static_cast<qint64>(wsObj.value("id").toDouble(-1));
		if (id < 0) continue;

		ids.push_back(id);

		auto* existing = this->findWorkspaceById(id);
		if (existing) {
			existing->updateFromJson(wsObj);
		} else {
			auto* workspace = new NiriWorkspace(this);
			workspace->updateFromJson(wsObj);
			this->mWorkspaces.insertObjectSorted(workspace, &NiriIpc::compareWorkspaces);
			qCDebug(logNiriIpc) << "New workspace:" << id;
		}
	}

	// Remove workspaces no longer present
	auto removedWorkspaces = QVector<NiriWorkspace*>();
	for (auto* workspace: mList) {
		if (!ids.contains(workspace->bindableId().value())) {
			removedWorkspaces.push_back(workspace);
		}
	}

	for (auto* workspace: removedWorkspaces) {
		this->mWorkspaces.removeObject(workspace);
		workspace->deleteLater();
	}

	// Re-sort since workspace data may have changed
	auto sortedList = this->mWorkspaces.valueList();
	std::ranges::sort(sortedList, [](NiriWorkspace* a, NiriWorkspace* b) {
		auto aOutput = a->bindableOutput().value();
		auto bOutput = b->bindableOutput().value();
		if (aOutput != bOutput) {
			return aOutput < bOutput;
		}
		return a->bindableIdx().value() < b->bindableIdx().value();
	});
	reorderNiriWorkspaceModel(this->mWorkspaces, sortedList);

	emit this->workspacesUpdated();
}

void NiriIpc::updateWindowsFromArray(const QJsonArray& windowsArray) {
	qCDebug(logNiriIpc) << "Updating windows";

	auto ids = QVector<qint64>();
	const auto& mList = this->mWindows.valueList();

	NiriWindow* newFocused = nullptr;

	for (const auto& entry: windowsArray) {
		auto winObj = entry.toObject();
		auto id = static_cast<qint64>(winObj.value("id").toDouble(-1));
		if (id < 0) continue;

		ids.push_back(id);

		auto workspaceId = static_cast<qint64>(winObj.value("workspace_id").toDouble(-1));
		auto outputName = this->resolveWindowOutput(workspaceId);

		auto* existing = this->findWindowById(id);
		if (existing) {
			existing->updateFromJson(winObj, outputName);
			if (existing->bindableFocused().value()) {
				newFocused = existing;
			}
		} else {
			auto* window = new NiriWindow(this);
			window->updateFromJson(winObj, outputName);
			this->mWindows.insertObject(window);
			if (window->bindableFocused().value()) {
				newFocused = window;
			}
			qCDebug(logNiriIpc) << "New window:" << id;
		}
	}

	// Remove windows no longer present
	auto removedWindows = QVector<NiriWindow*>();
	for (auto* window: mList) {
		if (!ids.contains(window->bindableId().value())) {
			removedWindows.push_back(window);
		}
	}

	for (auto* window: removedWindows) {
		this->mWindows.removeObject(window);
		window->deleteLater();
	}

	this->bFocusedWindow = newFocused;

	emit this->windowsUpdated();
}

bool NiriIpc::compareWorkspaces(NiriWorkspace* a, NiriWorkspace* b) {
	auto aOutput = a->bindableOutput().value();
	auto bOutput = b->bindableOutput().value();
	if (aOutput != bOutput) {
		return aOutput > bOutput;
	}
	return a->bindableIdx().value() > b->bindableIdx().value();
}

} // namespace qs::niri::ipc
