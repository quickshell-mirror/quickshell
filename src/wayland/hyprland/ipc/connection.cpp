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
#include <qproperty.h>
#include <qqml.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../../core/logcat.hpp"
#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "../../toplevel_management/handle.hpp"
#include "hyprland_toplevel.hpp"
#include "monitor.hpp"
#include "toplevel_mapping.hpp"
#include "workspace.hpp"

namespace qs::hyprland::ipc {

namespace {
QS_LOGGING_CATEGORY(logHyprlandIpc, "quickshell.hyprland.ipc", QtWarningMsg);
QS_LOGGING_CATEGORY(logHyprlandIpcEvents, "quickshell.hyprland.ipc.events", QtWarningMsg);
} // namespace

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

	this->bFocusedWorkspace.setBinding([this]() -> HyprlandWorkspace* {
		if (!this->bFocusedMonitor) return nullptr;
		return this->bFocusedMonitor->bindableActiveWorkspace().value();
	});

	this->mRequestSocketPath = hyprlandDir + "/.socket.sock";
	this->mEventSocketPath = hyprlandDir + "/.socket2.sock";

	// clang-format off
	QObject::connect(&this->eventSocket, &QLocalSocket::errorOccurred, this, &HyprlandIpc::eventSocketError);
	QObject::connect(&this->eventSocket, &QLocalSocket::stateChanged, this, &HyprlandIpc::eventSocketStateChanged);
	QObject::connect(&this->eventSocket, &QLocalSocket::readyRead, this, &HyprlandIpc::eventSocketReady);

	auto *instance = HyprlandToplevelMappingManager::instance();
	QObject::connect(instance, &HyprlandToplevelMappingManager::toplevelAddressed, this, &HyprlandIpc::toplevelAddressed);

	// clang-format on

	this->eventSocket.connectToServer(this->mEventSocketPath, QLocalSocket::ReadOnly);
	this->refreshMonitors(true);
	this->refreshWorkspaces(true);
	this->refreshToplevels();
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

void HyprlandIpc::toplevelAddressed(
    wayland::toplevel_management::impl::ToplevelHandle* handle,
    quint64 address
) {
	auto* waylandToplevel =
	    wayland::toplevel_management::ToplevelManager::instance()->forImpl(handle);

	if (!waylandToplevel) return;

	auto* attached = qobject_cast<HyprlandToplevel*>(
	    qmlAttachedPropertiesObject<HyprlandToplevel>(waylandToplevel, false)
	);

	auto* hyprToplevel = this->findToplevelByAddress(address, true);

	if (attached) {
		if (attached->address()) {
			qCDebug(logHyprlandIpc) << "Toplevel" << attached->addressStr() << "already has address"
			                        << address;

			return;
		}

		attached->setAddress(address);
		attached->setHyprlandHandle(hyprToplevel);
	}

	hyprToplevel->setWaylandHandle(waylandToplevel->implHandle());
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
		requestSocket->flush();
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

ObjectModel<HyprlandToplevel>* HyprlandIpc::toplevels() { return &this->mToplevels; }

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

	while (args.length() < count) {
		args.push_back(QByteArrayView());
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
		this->refreshToplevels();
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

		auto monitorIter = std::ranges::find_if(mList, [name](HyprlandMonitor* m) {
			return m->bindableName().value() == name;
		});

		if (monitorIter == mList.end()) {
			qCWarning(logHyprlandIpc) << "Got removal for monitor" << name
			                          << "which was not previously tracked.";
			return;
		}

		auto index = monitorIter - mList.begin();
		auto* monitor = *monitorIter;

		qCDebug(logHyprlandIpc) << "Monitor removed with id" << monitor->bindableId().value() << "name"
		                        << monitor->bindableName().value();
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
			this->mWorkspaces.insertObjectSorted(workspace, &HyprlandIpc::compareWorkspaces);
		}
	} else if (event->name == "destroyworkspacev2") {
		auto args = event->parseView(2);

		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		const auto& mList = this->mWorkspaces.valueList();

		auto workspaceIter = std::ranges::find_if(mList, [id](HyprlandWorkspace* m) {
			return m->bindableId().value() == id;
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
			if (monitor->bindableActiveWorkspace().value() == nullptr) {
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
		qCDebug(logHyprlandIpc) << "Monitor" << name << "focused with workspace"
		                        << (workspace ? workspace->bindableId().value() : -1);
	} else if (event->name == "workspacev2") {
		auto args = event->parseView(2);
		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		if (this->bFocusedMonitor != nullptr) {
			auto* workspace = this->findWorkspaceByName(name, true, id);
			this->bFocusedMonitor->setActiveWorkspace(workspace);
			qCDebug(logHyprlandIpc) << "Workspace" << id << "activated on"
			                        << this->bFocusedMonitor->bindableName().value();
		}
	} else if (event->name == "moveworkspacev2") {
		auto args = event->parseView(3);
		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));
		auto monitorName = QString::fromUtf8(args.at(2));

		auto* workspace = this->findWorkspaceByName(name, true, id);
		auto* monitor = this->findMonitorByName(monitorName, true);

		qCDebug(logHyprlandIpc) << "Workspace" << id << "moved to monitor" << monitorName;
		workspace->setMonitor(monitor);
	} else if (event->name == "renameworkspace") {
		auto args = event->parseView(2);
		auto id = args.at(0).toInt();
		auto name = QString::fromUtf8(args.at(1));

		const auto& mList = this->mWorkspaces.valueList();

		auto workspaceIter = std::ranges::find_if(mList, [id](HyprlandWorkspace* m) {
			return m->bindableId().value() == id;
		});

		if (workspaceIter == mList.end()) return;

		qCDebug(logHyprlandIpc) << "Workspace with id" << id << "renamed from"
		                        << (*workspaceIter)->bindableName().value() << "to" << name;

		(*workspaceIter)->bindableName().setValue(name);
	} else if (event->name == "fullscreen") {
		if (auto* workspace = this->bFocusedWorkspace.value()) {
			workspace->bindableHasFullscreen().setValue(event->data == "1");
		}

		// In theory knowing the current workspace would be enough to determine where
		// the fullscreen state changed, but this falls apart if you move a fullscreen
		// window between workspaces.
		this->refreshWorkspaces(false);
	} else if (event->name == "openwindow") {
		auto args = event->parseView(4);
		auto ok = false;
		auto windowAddress = args.at(0).toULongLong(&ok, 16);

		if (!ok) return;

		auto workspaceName = QString::fromUtf8(args.at(1));
		auto windowTitle = QString::fromUtf8(args.at(2));
		auto windowClass = QString::fromUtf8(args.at(3));

		auto* workspace = this->findWorkspaceByName(workspaceName, false);
		if (!workspace) {
			qCWarning(logHyprlandIpc) << "Got openwindow for workspace" << workspaceName
			                          << "which was not previously tracked.";
			return;
		}

		auto* toplevel = this->findToplevelByAddress(windowAddress, false);
		const bool existed = toplevel != nullptr;

		if (!toplevel) toplevel = new HyprlandToplevel(this);
		toplevel->updateInitial(windowAddress, windowTitle, workspaceName);

		workspace->insertToplevel(toplevel);

		if (!existed) {
			this->mToplevels.insertObject(toplevel);
			qCDebug(logHyprlandIpc) << "New toplevel created with address" << windowAddress << ", title"
			                        << windowTitle << ", workspace" << workspaceName;
		}
	} else if (event->name == "closewindow") {
		auto args = event->parseView(1);
		auto ok = false;
		auto windowAddress = args.at(0).toULongLong(&ok, 16);

		if (!ok) return;

		const auto& mList = this->mToplevels.valueList();
		auto toplevelIter = std::ranges::find_if(mList, [windowAddress](HyprlandToplevel* m) {
			return m->address() == windowAddress;
		});

		if (toplevelIter == mList.end()) {
			qCWarning(logHyprlandIpc) << "Got closewindow for address" << windowAddress
			                          << "which was not previously tracked.";
			return;
		}

		auto* toplevel = *toplevelIter;
		auto index = toplevelIter - mList.begin();
		this->mToplevels.removeAt(index);

		// Remove from workspace
		auto* workspace = toplevel->bindableWorkspace().value();
		if (workspace) {
			workspace->toplevels()->removeObject(toplevel);
		}

		delete toplevel;
	} else if (event->name == "movewindowv2") {
		auto args = event->parseView(3);
		auto ok = false;
		auto windowAddress = args.at(0).toULongLong(&ok, 16);
		auto workspaceName = QString::fromUtf8(args.at(2));

		auto* toplevel = this->findToplevelByAddress(windowAddress, false);
		if (!toplevel) {
			qCWarning(logHyprlandIpc) << "Got movewindowv2 event for client with address" << windowAddress
			                          << "which was not previously tracked.";
			return;
		}

		HyprlandWorkspace* workspace = this->findWorkspaceByName(workspaceName, false);
		if (!workspace) {
			qCWarning(logHyprlandIpc) << "Got movewindowv2 event for workspace" << args.at(2)
			                          << "which was not previously tracked.";
			return;
		}

		auto* oldWorkspace = toplevel->bindableWorkspace().value();
		toplevel->setWorkspace(workspace);

		if (oldWorkspace) {
			oldWorkspace->removeToplevel(toplevel);
		}

		workspace->insertToplevel(toplevel);
	} else if (event->name == "windowtitlev2") {
		auto args = event->parseView(2);
		auto ok = false;
		auto windowAddress = args.at(0).toULongLong(&ok, 16);
		auto windowTitle = QString::fromUtf8(args.at(1));

		if (!ok) return;

		// It happens that Hyprland sends windowtitlev2 events before event
		// "openwindow" is emitted, so let's preemptively create it
		auto* toplevel = this->findToplevelByAddress(windowAddress, true);
		if (!toplevel) {
			qCWarning(logHyprlandIpc) << "Got windowtitlev2 event for client with address"
			                          << windowAddress << "which was not previously tracked.";
			return;
		}

		toplevel->bindableTitle().setValue(windowTitle);
	} else if (event->name == "activewindowv2") {
		auto args = event->parseView(1);
		auto ok = false;
		auto windowAddress = args.at(0).toULongLong(&ok, 16);

		if (!ok) return;

		// Did not observe "activewindowv2" event before "openwindow",
		// but better safe than sorry, so create if missing.
		auto* toplevel = this->findToplevelByAddress(windowAddress, true);
		this->bActiveToplevel = toplevel;
	} else if (event->name == "urgent") {
		auto args = event->parseView(1);
		auto ok = false;
		auto windowAddress = args.at(0).toULongLong(&ok, 16);

		if (!ok) return;

		// It happens that Hyprland sends urgent before "openwindow"
		auto* toplevel = this->findToplevelByAddress(windowAddress, true);
		toplevel->bindableUrgent().setValue(true);
	}
}

HyprlandWorkspace*
HyprlandIpc::findWorkspaceByName(const QString& name, bool createIfMissing, qint32 id) {
	const auto& mList = this->mWorkspaces.valueList();
	HyprlandWorkspace* workspace = nullptr;

	if (id != -1) {
		auto workspaceIter = std::ranges::find_if(mList, [&](HyprlandWorkspace* m) {
			return m->bindableId().value() == id;
		});

		workspace = workspaceIter == mList.end() ? nullptr : *workspaceIter;
	}

	if (!workspace) {
		auto workspaceIter = std::ranges::find_if(mList, [&](HyprlandWorkspace* m) {
			return m->bindableName().value() == name;
		});

		workspace = workspaceIter == mList.end() ? nullptr : *workspaceIter;
	}

	if (workspace) {
		return workspace;
	} else if (createIfMissing) {
		qCDebug(logHyprlandIpc) << "Workspace" << name
		                        << "requested before creation, performing early init with id" << id;

		auto* workspace = new HyprlandWorkspace(this);
		workspace->updateInitial(id, name);
		this->mWorkspaces.insertObjectSorted(workspace, &HyprlandIpc::compareWorkspaces);
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

		qCDebug(logHyprlandIpc) << "Parsing workspaces response";
		auto json = QJsonDocument::fromJson(resp).array();

		const auto& mList = this->mWorkspaces.valueList();
		auto ids = QVector<quint32>();

		for (auto entry: json) {
			auto object = entry.toObject().toVariantMap();

			auto id = object.value("id").toInt();

			auto workspaceIter = std::ranges::find_if(mList, [&](HyprlandWorkspace* m) {
				return m->bindableId().value() == id;
			});

			// Only fall back to name-based filtering as a last resort, for workspaces where
			// no ID has been determined yet.
			if (workspaceIter == mList.end()) {
				auto name = object.value("name").toString();

				workspaceIter = std::ranges::find_if(mList, [&](HyprlandWorkspace* m) {
					return m->bindableId().value() == -1 && m->bindableName().value() == name;
				});
			}

			auto* workspace = workspaceIter == mList.end() ? nullptr : *workspaceIter;
			auto existed = workspace != nullptr;

			if (!existed) {
				if (!canCreate) continue;
				workspace = new HyprlandWorkspace(this);
			}

			workspace->updateFromObject(object);

			if (!existed) {
				this->mWorkspaces.insertObjectSorted(workspace, &HyprlandIpc::compareWorkspaces);
			}

			ids.push_back(id);
		}

		if (canCreate) {
			auto removedWorkspaces = QVector<HyprlandWorkspace*>();

			for (auto* workspace: mList) {
				if (!ids.contains(workspace->bindableId().value())) {
					removedWorkspaces.push_back(workspace);
				}
			}

			for (auto* workspace: removedWorkspaces) {
				this->mWorkspaces.removeObject(workspace);
				delete workspace;
			}
		}
	});
}

HyprlandToplevel* HyprlandIpc::findToplevelByAddress(quint64 address, bool createIfMissing) {
	const auto& mList = this->mToplevels.valueList();
	HyprlandToplevel* toplevel = nullptr;

	auto toplevelIter =
	    std::ranges::find_if(mList, [&](HyprlandToplevel* m) { return m->address() == address; });

	toplevel = toplevelIter == mList.end() ? nullptr : *toplevelIter;

	if (!toplevel && createIfMissing) {
		qCDebug(logHyprlandIpc) << "Toplevel with address" << address
		                        << "requested before creation, performing early init";

		toplevel = new HyprlandToplevel(this);
		toplevel->updateInitial(address, "", "");
		this->mToplevels.insertObject(toplevel);
	}

	return toplevel;
}

void HyprlandIpc::refreshToplevels() {
	if (this->requestingToplevels) return;
	this->requestingToplevels = true;

	this->makeRequest("j/clients", [this](bool success, const QByteArray& resp) {
		this->requestingToplevels = false;
		if (!success) return;

		qCDebug(logHyprlandIpc) << "Parsing j/clients response";
		auto json = QJsonDocument::fromJson(resp).array();

		const auto& mList = this->mToplevels.valueList();

		for (auto entry: json) {
			auto object = entry.toObject().toVariantMap();

			bool ok = false;
			auto address = object.value("address").toString().toULongLong(&ok, 16);

			if (!ok) {
				qCWarning(logHyprlandIpc) << "Invalid address in j/clients entry:" << object;
				continue;
			}

			auto toplevelsIter =
			    std::ranges::find_if(mList, [&](HyprlandToplevel* m) { return m->address() == address; });

			auto* toplevel = toplevelsIter == mList.end() ? nullptr : *toplevelsIter;
			auto exists = toplevel != nullptr;

			if (!exists) toplevel = new HyprlandToplevel(this);
			toplevel->updateFromObject(object);

			if (!exists) {
				qCDebug(logHyprlandIpc) << "New toplevel created with address" << address;
				this->mToplevels.insertObject(toplevel);
			}

			auto* workspace = toplevel->bindableWorkspace().value();
			workspace->insertToplevel(toplevel);
		}
	});
}

HyprlandMonitor*
HyprlandIpc::findMonitorByName(const QString& name, bool createIfMissing, qint32 id) {
	const auto& mList = this->mMonitors.valueList();

	auto monitorIter = std::ranges::find_if(mList, [name](HyprlandMonitor* m) {
		return m->bindableName().value() == name;
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

HyprlandMonitor* HyprlandIpc::monitorFor(QuickshellScreenInfo* screen) {
	// Wayland monitors appear after hyprland ones are created and disappear after destruction
	// so simply not doing any preemptive creation is enough, however if this call creates
	// the HyprlandIpc singleton then monitors won't be initialized, in which case we
	// preemptively create one.

	if (screen == nullptr) return nullptr;
	return this->findMonitorByName(screen->name(), !this->monitorsRequested);
}

void HyprlandIpc::setFocusedMonitor(HyprlandMonitor* monitor) {
	auto* oldMonitor = this->bFocusedMonitor.value();
	if (monitor == oldMonitor) return;

	if (this->bFocusedMonitor != nullptr) {
		QObject::disconnect(this->bFocusedMonitor, nullptr, this, nullptr);
	}

	if (monitor != nullptr) {
		QObject::connect(monitor, &QObject::destroyed, this, &HyprlandIpc::onFocusedMonitorDestroyed);
	}

	this->bFocusedMonitor = monitor;
}

void HyprlandIpc::onFocusedMonitorDestroyed() {
	this->bFocusedMonitor = nullptr;
	emit this->focusedMonitorChanged();
}

void HyprlandIpc::refreshMonitors(bool canCreate) {
	if (this->requestingMonitors) return;
	this->requestingMonitors = true;

	this->makeRequest("j/monitors", [this, canCreate](bool success, const QByteArray& resp) {
		this->requestingMonitors = false;
		if (!success) return;

		this->monitorsRequested = true;

		qCDebug(logHyprlandIpc) << "parsing monitors response";
		auto json = QJsonDocument::fromJson(resp).array();

		const auto& mList = this->mMonitors.valueList();
		auto names = QVector<QString>();

		for (auto entry: json) {
			auto object = entry.toObject().toVariantMap();
			auto name = object.value("name").toString();

			auto monitorIter = std::ranges::find_if(mList, [name](HyprlandMonitor* m) {
				return m->bindableName().value() == name;
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

			names.push_back(name);
		}

		auto removedMonitors = QVector<HyprlandMonitor*>();

		for (auto* monitor: mList) {
			if (!names.contains(monitor->bindableName().value())) {
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

bool HyprlandIpc::compareWorkspaces(HyprlandWorkspace* a, HyprlandWorkspace* b) {
	return a->bindableId().value() > b->bindableId().value();
}

} // namespace qs::hyprland::ipc
