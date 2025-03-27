#pragma once

#include <qbytearrayview.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqml.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"

namespace qs::i3::ipc {

class I3Workspace;
class I3Monitor;
} // namespace qs::i3::ipc

Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Workspace*);
Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Monitor*);

namespace qs::i3::ipc {

constexpr std::string MAGIC = "i3-ipc";

enum EventCode {
	RunCommand = 0,
	GetWorkspaces = 1,
	Subscribe = 2,
	GetOutputs = 3,
	GetTree = 4,

	Workspace = 0x80000000,
	Output = 0x80000001,
	Mode = 0x80000002,
	Window = 0x80000003,
	BarconfigUpdate = 0x80000004,
	Binding = 0x80000005,
	Shutdown = 0x80000006,
	Tick = 0x80000007,
	BarStateUpdate = 0x80000014,
	Input = 0x80000015,
	Unknown = 999,
};

using Event = std::tuple<EventCode, QJsonDocument>;

///! I3/Sway IPC Events
/// Emitted by @@I3.rawEvent(s)
class I3IpcEvent: public QObject {
	Q_OBJECT;

	/// The name of the event
	Q_PROPERTY(QString type READ type CONSTANT);
	/// The payload of the event in JSON format.
	Q_PROPERTY(QString data READ data CONSTANT);

	QML_NAMED_ELEMENT(I3Event);
	QML_UNCREATABLE("I3IpcEvents cannot be created.");

public:
	I3IpcEvent(QObject* parent): QObject(parent) {}

	[[nodiscard]] QString type() const;
	[[nodiscard]] QString data() const;

	EventCode mCode = EventCode::Unknown;
	QJsonDocument mData;

	static EventCode intToEvent(uint32_t raw);
	static QString eventToString(EventCode event);
};

class I3Ipc: public QObject {
	Q_OBJECT;

public:
	static I3Ipc* instance();

	[[nodiscard]] QString socketPath() const;

	void makeRequest(const QByteArray& request);
	void dispatch(const QString& payload);

	static QByteArray buildRequestMessage(EventCode cmd, const QByteArray& payload = QByteArray());

	I3Workspace* findWorkspaceByName(const QString& name);
	I3Monitor* findMonitorByName(const QString& name, bool createIfMissing = false);
	I3Workspace* findWorkspaceByID(qint32 id);

	void setFocusedMonitor(I3Monitor* monitor);

	void refreshWorkspaces();
	void refreshMonitors();

	I3Monitor* monitorFor(QuickshellScreenInfo* screen);

	[[nodiscard]] QBindable<I3Monitor*> bindableFocusedMonitor() const {
		return &this->bFocusedMonitor;
	};

	[[nodiscard]] QBindable<I3Workspace*> bindableFocusedWorkspace() const {
		return &this->bFocusedWorkspace;
	};

	[[nodiscard]] ObjectModel<I3Monitor>* monitors();
	[[nodiscard]] ObjectModel<I3Workspace>* workspaces();

signals:
	void connected();
	void rawEvent(I3IpcEvent* event);
	void focusedWorkspaceChanged();
	void focusedMonitorChanged();

private slots:
	void eventSocketError(QLocalSocket::LocalSocketError error) const;
	void eventSocketStateChanged(QLocalSocket::LocalSocketState state);
	void eventSocketReady();
	void subscribe();

	void onFocusedMonitorDestroyed();

private:
	explicit I3Ipc();

	void onEvent(I3IpcEvent* event);

	void handleWorkspaceEvent(I3IpcEvent* event);
	void handleGetWorkspacesEvent(I3IpcEvent* event);
	void handleGetOutputsEvent(I3IpcEvent* event);
	static void handleRunCommand(I3IpcEvent* event);
	static bool compareWorkspaces(I3Workspace* a, I3Workspace* b);

	void reconnectIPC();

	QVector<std::tuple<EventCode, QJsonDocument>> parseResponse();

	QLocalSocket liveEventSocket;
	QDataStream liveEventSocketDs;

	QString mSocketPath;

	bool valid = false;

	ObjectModel<I3Monitor> mMonitors {this};
	ObjectModel<I3Workspace> mWorkspaces {this};

	I3IpcEvent event {this};

	Q_OBJECT_BINDABLE_PROPERTY(I3Ipc, I3Monitor*, bFocusedMonitor, &I3Ipc::focusedMonitorChanged);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3Ipc,
	    I3Workspace*,
	    bFocusedWorkspace,
	    &I3Ipc::focusedWorkspaceChanged
	);
};

} // namespace qs::i3::ipc
