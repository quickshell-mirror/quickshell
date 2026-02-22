#pragma once

#include <qbytearrayview.h>
#include <qdatastream.h>
#include <qjsondocument.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::i3::ipc {

constexpr std::string MAGIC = "i3-ipc";

enum EventCode {
	RunCommand = 0,
	GetWorkspaces = 1,
	Subscribe = 2,
	GetOutputs = 3,
	GetTree = 4,
	GetMarks = 5,
	GetVersion = 7,
	GetBindingModes = 8,
	GetBindingState = 12,
	GetInputs = 100,
	GetScroller = 120,
	GetTrails = 121,
	GetSpaces = 122,
	GetBindings = 123,

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
	Lua = 0x8000001d,
	Scroller = 0x8000001e,
	Trails = 0x8000001f,
	Unknown = 999,
};

using Event = std::tuple<EventCode, QJsonDocument>;

///! I3/Sway IPC Events
/// Emitted by @@I3.rawEvent(s)
class I3IpcEvent: public QObject {
	Q_OBJECT;

	Q_PROPERTY(QString type READ type CONSTANT);
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

/// Base class that manages the IPC socket, subscriptions and event reception.
class I3Ipc: public QObject {
	Q_OBJECT;

public:
	explicit I3Ipc(const QList<QString>& events);

	[[nodiscard]] QString socketPath() const;
	[[nodiscard]] QString compositor() const;

	void makeRequest(const QByteArray& request);
	void dispatch(const QString& payload);
	void connect();

	[[nodiscard]] QByteArray static buildRequestMessage(
	    EventCode cmd,
	    const QByteArray& payload = QByteArray()
	);

signals:
	void connected();
	void rawEvent(I3IpcEvent* event);

protected slots:
	void eventSocketError(QLocalSocket::LocalSocketError error) const;
	void eventSocketStateChanged(QLocalSocket::LocalSocketState state);
	void eventSocketReady();
	void subscribe();

protected:
	void reconnectIPC();
	QVector<std::tuple<EventCode, QJsonDocument>> parseResponse();

	QLocalSocket liveEventSocket;
	QDataStream liveEventSocketDs;

	QString mSocketPath;
	bool valid = false;

	I3IpcEvent event {this};

private:
	QList<QString> mEvents;
	QString mCompositor;
};

} // namespace qs::i3::ipc
