#include "connection.hpp"
#include <algorithm>
#include <functional>
#include <utility>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "monitor.hpp"
#include "workspace.hpp"

namespace qs::hyprland::ipc {

Q_LOGGING_CATEGORY(logHyprlandIpc, "quickshell.hyprland.ipc", QtWarningMsg);
Q_LOGGING_CATEGORY(logHyprlandIpcEvents, "quickshell.hyprland.ipc.events", QtWarningMsg);

HyprlandIpc::HyprlandIpc() {
	auto his = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
	if (his.isEmpty()) {
		qWarning() << "$HYPRLAND_INSTANCE_SIGNATURE is unset. Cannot connect to hyprland.";
		return;
	}

	auto runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
	auto hyprlandDir = runtimeDir + "/hypr/" + his;

	if (!QFileInfo(hyprlandDir).isDir()) {
		hyprlandDir = "/tmp/hypr/" + his;
	}

	if (!QFileInfo(hyprlandDir).isDir()) {
		qWarning() << "Unable to find hyprland socket. Cannot connect to hyprland.";
		return;
	}

	this->mRequestSocketPath = hyprlandDir + "/.socket.sock";
	this->mEventSocketPath = hyprlandDir + "/.socket2.sock";

	// clang-format off
	QObject::connect(&this->eventSocket, &QLocalSocket::errorOccurred, this, &HyprlandIpc::eventSocketError);
	QObject::connect(&this->eventSocket, &QLocalSocket::stateChanged, this, &HyprlandIpc::eventSocketStateChanged);
	QObject::connect(&this->eventSocket, &QLocalSocket::readyRead, this, &HyprlandIpc::eventSocketReady);
	// clang-format on

	// Sockets don't appear to be able to send data in the first event loop
	// cycle of the program, so delay it by one. No idea why this is the case.
	QTimer::singleShot(0, [this]() {
		this->eventSocket.connectToServer(this->mEventSocketPath, QLocalSocket::ReadOnly);
		this->refreshMonitors(true);
		this->refreshWorkspaces(true);
	});
}

QString HyprlandIpc::requestSocketPath() const { return this->mRequestSocketPath; }
QString HyprlandIpc::eventSocketPath() const { return this->mEventSocketPath; }

void HyprlandIpc::eventSocketError(QLocalSocket::LocalSocketError error) const {
	if (!this->valid) {
		qWarning() << "Unable to connect to hyprland event socket:" << error;
	} else {
		qWarning() << "Hyprland event socket error:" << error;
	}
}

void HyprlandIpc::eventSocketStateChanged(QLocalSocket::LocalSocketState state) {
	if (state == QLocalSocket::ConnectedState) {
		qCInfo(logHyprlandIpc) << "Hyprland event socket connected.";
		emit this->connected();
	} else if (state == QLocalSocket::UnconnectedState && this->valid) {
		qCWarning(logHyprlandIpc) << "Hyprland event socket disconnected.";
	}

	this->valid = state == QLocalSocket::ConnectedState;
}

void HyprlandIpc::eventSocketReady() {
	while (true) {
		auto rawEvent = this->eventSocket.readLine();
		if (rawEvent.isEmpty()) break;

		// remove trailing \n
		rawEvent.truncate(rawEvent.length() - 1);
		auto splitIdx = rawEvent.indexOf(">>");
		auto event = QByteArrayView(rawEvent.data(), splitIdx);
		auto data = QByteArrayView(
		    rawEvent.data() + splitIdx + 2,     // NOLINT
		    rawEvent.data() + rawEvent.length() // NOLINT
		);
		qCDebug(logHyprlandIpcEvents) << "Received event:" << rawEvent << "parsed as" << event << data;

		this->event.name = event;
		this->event.data = data;
		this->onEvent(&this->event);
		emit this->rawEvent(&this->event);
	}
}

void HyprlandIpc::makeRequest(
    const QByteArray& request,
    const std::function<void(bool, QByteArray)>& callback
) {
	auto* requestSocket = new QLocalSocket(this);
	qCDebug(logHyprlandIpc) << "Making request:" << request;

	auto connectedCallback = [this, request, requestSocket, callback]() {
		auto responseCallback = [requestSocket, callback]() {
			auto response = requestSocket->readAll();
			callback(true, std::move(response));
			delete requestSocket;
		};

		QObject::connect(requestSocket, &QLocalSocket::readyRead, this, responseCallback);

		requestSocket->write(request);
	};

	auto errorCallback = [=](QLocalSocket::LocalSocketError error) {
		qCWarning(logHyprlandIpc) << "Error making request:" << error << "request:" << request;
		requestSocket->deleteLater();
		callback(false, {});
	};

	QObject::connect(requestSocket, &QLocalSocket::connected, this, connectedCallback);
	QObject::connect(requestSocket, &QLocalSocket::errorOccurred, this, errorCallback);

	requestSocket->connectToServer(this->mRequestSocketPath);
}

void HyprlandIpc::dispatch(const QString& request) {
	this->makeRequest(
	    ("dispatch " + request).toUtf8(),
	    [request](bool success, const QByteArray& response) {
		    if (!success) {
			    qCWarning(logHyprlandIpc) << "Failed to request dispatch of" << request;
			    return;
		    }

		    if (response != "ok") {
			    qCWarning(logHyprlandIpc)
			        << "Dispatch request" << request << "failed with error" << response;
		    }
	    }
	);
}

ObjectModel<HyprlandMonitor>* HyprlandIpc::monitors() { return &this->mMonitors; }

ObjectModel<HyprlandWorkspace>* HyprlandIpc::workspaces() { return &this->mWorkspaces; }

QVector<QByteArrayView> HyprlandIpc::parseEventArgs(QByteArrayView event, quint16 count) {
	auto args = QVector<QByteArrayView>();

	for (auto i = 0; i < count - 1; i++) {
		auto splitIdx = event.indexOf(',');
		if (splitIdx == -1) break;
		args.push_back(event.sliced(0, splitIdx));
		event = event.sliced(splitIdx + 1);
	}

	if (!event.isEmpty()) {
		args.push_back(event);
	}

	return args;
}

QVector<QString> HyprlandIpcEvent::parse(qint32 argumentCount) const {
	auto args = QVector<QString>();

	for (auto arg: this->parseView(argumentCount)) {
		args.push_back(QString::fromUtf8(arg));
	}

	return args;
}

QVector<QByteArrayView> HyprlandIpcEvent::parseView(qint32 argumentCount) const {
	return HyprlandIpc::parseEventArgs(this->data, argumentCount);
}

QString HyprlandIpcEvent::nameStr() const { return QString::fromUtf8(this->name); }
QString HyprlandIpcEvent::dataStr() const { return QString::fromUtf8(this->data); }

HyprlandIpc* HyprlandIpc::instance() {
	static HyprlandIpc* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new HyprlandIpc();
	}

	return instance;
}

void HyprlandIpc::onEvent(HyprlandIpcEvent* event) {
	if (event->name == "configreloaded") {
		this->refreshMonitors(true);
		this->refreshWorkspaces(true);
	} else if (event->name == "monitoraddedv2") {
		auto args = event->parseView(3);

		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		// hyprland will often reference the monitor before creation, in which case
		// it will already exist.
		auto* monitor = this->findMonitorByName(name, false);
		auto existed = monitor != nullptr;

		if (monitor == nullptr) {
			monitor = new HyprlandMonitor(this);
		}

		qCDebug(logHyprlandIpc) << "Monitor added with id" << id << "name" << name
		                        << "preemptively created:" << existed;

		monitor->updateInitial(id, name, QString::fromUtf8(args.at(2)));

		if (!existed) {
			this->mMonitors.insertObject(monitor);
		}

		// refresh even if it already existed because workspace focus might have changed.
		this->refreshMonitors(false);
	} else if (event->name == "monitorremoved") {
		const auto& mList = this->mMonitors.valueList();
		auto name = QString::fromUtf8(event->data);

		auto monitorIter = std::find_if(mList.begin(), mList.end(), [name](const HyprlandMonitor* m) {
			return m->name() == name;
		});

		if (monitorIter == mList.end()) {
			qCWarning(logHyprlandIpc) << "Got removal for monitor" << name
			                          << "which was not previously tracked.";
			return;
		}

		auto index = monitorIter - mList.begin();
		auto* monitor = *monitorIter;

		qCDebug(logHyprlandIpc) << "Monitor removed with id" << monitor->id() << "name"
		                        << monitor->name();
		this->mMonitors.removeAt(index);

		// delete the monitor object in the next event loop cycle so it's likely to
		// still exist when future events reference it after destruction.
		// If we get to the next cycle and things still reference it (unlikely), nulls
		// can make it to the frontend.
		monitor->deleteLater();
	} else if (event->name == "createworkspacev2") {
		auto args = event->parseView(2);

		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		qCDebug(logHyprlandIpc) << "Workspace created with id" << id << "name" << name;

		auto* workspace = this->findWorkspaceByName(name, false);
		auto existed = workspace != nullptr;

		if (workspace == nullptr) {
			workspace = new HyprlandWorkspace(this);
		}

		workspace->updateInitial(id, name);

		if (!existed) {
			this->refreshWorkspaces(false);
			this->mWorkspaces.insertObject(workspace);
		}
	} else if (event->name == "destroyworkspacev2") {
		auto args = event->parseView(2);

		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		const auto& mList = this->mWorkspaces.valueList();

		auto workspaceIter = std::find_if(mList.begin(), mList.end(), [id](const HyprlandWorkspace* m) {
			return m->id() == id;
		});

		if (workspaceIter == mList.end()) {
			qCWarning(logHyprlandIpc) << "Got removal for workspace id" << id << "name" << name
			                          << "which was not previously tracked.";
			return;
		}

		auto index = workspaceIter - mList.begin();
		auto* workspace = *workspaceIter;

		qCDebug(logHyprlandIpc) << "Workspace removed with id" << id << "name" << name;
		this->mWorkspaces.removeAt(index);

		// workspaces have not been observed to be referenced after deletion
		delete workspace;

		for (auto* monitor: this->mMonitors.valueList()) {
			if (monitor->activeWorkspace() == nullptr) {
				// removing a monitor will cause a new workspace to be created and destroyed after removal,
				// but it won't go back to a real workspace afterwards and just leaves a null, so we
				// re-query monitors if this appears to be the case.
				this->refreshMonitors(false);
				break;
			}
		}
	} else if (event->name == "focusedmon") {
		auto args = event->parseView(2);
		auto name = QString::fromUtf8(args.at(0));
		auto workspaceName = QString::fromUtf8(args.at(1));

		HyprlandWorkspace* workspace = nullptr;
		if (workspaceName != "?") { // what the fuck
			workspace = this->findWorkspaceByName(workspaceName, false);
		}

		auto* monitor = this->findMonitorByName(name, true);
		this->setFocusedMonitor(monitor);
		monitor->setActiveWorkspace(workspace);
	} else if (event->name == "workspacev2") {
		auto args = event->parseView(2);
		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		if (this->mFocusedMonitor != nullptr) {
			auto* workspace = this->findWorkspaceByName(name, true, id);
			this->mFocusedMonitor->setActiveWorkspace(workspace);
		}
	} else if (event->name == "moveworkspacev2") {
		auto args = event->parseView(3);
		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));
		auto monitorName = QString::fromUtf8(args.at(2));

		auto* workspace = this->findWorkspaceByName(name, true, id);
		auto* monitor = this->findMonitorByName(monitorName, true);

		workspace->setMonitor(monitor);
	}
}

HyprlandWorkspace*
HyprlandIpc::findWorkspaceByName(const QString& name, bool createIfMissing, qint32 id) {
	const auto& mList = this->mWorkspaces.valueList();

	auto workspaceIter = std::find_if(mList.begin(), mList.end(), [name](const HyprlandWorkspace* m) {
		return m->name() == name;
	});

	if (workspaceIter != mList.end()) {
		return *workspaceIter;
	} else if (createIfMissing) {
		qCDebug(logHyprlandIpc) << "Workspace" << name
		                        << "requested before creation, performing early init";
		auto* workspace = new HyprlandWorkspace(this);
		workspace->updateInitial(id, name);
		this->mWorkspaces.insertObject(workspace);
		return workspace;
	} else {
		return nullptr;
	}
}

void HyprlandIpc::refreshWorkspaces(bool canCreate) {
	if (this->requestingWorkspaces) return;
	this->requestingWorkspaces = true;

	this->makeRequest("j/workspaces", [this, canCreate](bool success, const QByteArray& resp) {
		this->requestingWorkspaces = false;
		if (!success) return;

		qCDebug(logHyprlandIpc) << "parsing workspaces response";
		auto json = QJsonDocument::fromJson(resp).array();

		const auto& mList = this->mWorkspaces.valueList();
		auto names = QVector<QString>();

		for (auto entry: json) {
			auto object = entry.toObject().toVariantMap();
			auto name = object.value("name").toString();

			auto workspaceIter =
			    std::find_if(mList.begin(), mList.end(), [name](const HyprlandWorkspace* m) {
				    return m->name() == name;
			    });

			auto* workspace = workspaceIter == mList.end() ? nullptr : *workspaceIter;
			auto existed = workspace != nullptr;

			if (workspace == nullptr) {
				if (!canCreate) continue;
				workspace = new HyprlandWorkspace(this);
			}

			workspace->updateFromObject(object);

			if (!existed) {
				this->mWorkspaces.insertObject(workspace);
			}

			names.push_back(name);
		}

		auto removedWorkspaces = QVector<HyprlandWorkspace*>();

		for (auto* workspace: mList) {
			if (!names.contains(workspace->name())) {
				removedWorkspaces.push_back(workspace);
			}
		}

		for (auto* workspace: removedWorkspaces) {
			this->mWorkspaces.removeObject(workspace);
			delete workspace;
		}
	});
}

HyprlandMonitor*
HyprlandIpc::findMonitorByName(const QString& name, bool createIfMissing, qint32 id) {
	const auto& mList = this->mMonitors.valueList();

	auto monitorIter = std::find_if(mList.begin(), mList.end(), [name](const HyprlandMonitor* m) {
		return m->name() == name;
	});

	if (monitorIter != mList.end()) {
		return *monitorIter;
	} else if (createIfMissing) {
		qCDebug(logHyprlandIpc) << "Monitor" << name
		                        << "requested before creation, performing early init";
		auto* monitor = new HyprlandMonitor(this);
		monitor->updateInitial(id, name, "");
		this->mMonitors.insertObject(monitor);
		return monitor;
	} else {
		return nullptr;
	}
}

HyprlandMonitor* HyprlandIpc::focusedMonitor() const { return this->mFocusedMonitor; }

HyprlandMonitor* HyprlandIpc::monitorFor(QuickshellScreenInfo* screen) {
	// Wayland monitors appear after hyprland ones are created and disappear after destruction
	// so simply not doing any preemptive creation is enough.

	if (screen == nullptr) return nullptr;
	return this->findMonitorByName(screen->name(), false);
}

void HyprlandIpc::setFocusedMonitor(HyprlandMonitor* monitor) {
	if (monitor == this->mFocusedMonitor) return;

	if (this->mFocusedMonitor != nullptr) {
		QObject::disconnect(this->mFocusedMonitor, nullptr, this, nullptr);
	}

	this->mFocusedMonitor = monitor;

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &HyprlandIpc::onFocusedMonitorDestroyed);
	}
	emit this->focusedMonitorChanged();
}

void HyprlandIpc::onFocusedMonitorDestroyed() {
	this->mFocusedMonitor = nullptr;
	emit this->focusedMonitorChanged();
}

void HyprlandIpc::refreshMonitors(bool canCreate) {
	if (this->requestingMonitors) return;
	this->requestingMonitors = true;

	this->makeRequest("j/monitors", [this, canCreate](bool success, const QByteArray& resp) {
		this->requestingMonitors = false;
		if (!success) return;

		qCDebug(logHyprlandIpc) << "parsing monitors response";
		auto json = QJsonDocument::fromJson(resp).array();

		const auto& mList = this->mMonitors.valueList();
		auto ids = QVector<qint32>();

		for (auto entry: json) {
			auto object = entry.toObject().toVariantMap();
			auto id = object.value("id").toInt();

			auto monitorIter = std::find_if(mList.begin(), mList.end(), [id](const HyprlandMonitor* m) {
				return m->id() == id;
			});

			auto* monitor = monitorIter == mList.end() ? nullptr : *monitorIter;
			auto existed = monitor != nullptr;

			if (monitor == nullptr) {
				if (!canCreate) continue;
				monitor = new HyprlandMonitor(this);
			}

			monitor->updateFromObject(object);

			if (!existed) {
				this->mMonitors.insertObject(monitor);
			}

			ids.push_back(id);
		}

		auto removedMonitors = QVector<HyprlandMonitor*>();

		for (auto* monitor: mList) {
			if (!ids.contains(monitor->id())) {
				removedMonitors.push_back(monitor);
			}
		}

		for (auto* monitor: removedMonitors) {
			this->mMonitors.removeObject(monitor);
			// see comment in onEvent
			monitor->deleteLater();
		}
	});
}

} // namespace qs::hyprland::ipc
