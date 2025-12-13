#include "connection.hpp"
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

namespace qs::i3::ipc {

namespace {
QS_LOGGING_CATEGORY(logI3Ipc, "quickshell.I3.ipc", QtWarningMsg);
QS_LOGGING_CATEGORY(logI3IpcEvents, "quickshell.I3.ipc.events", QtWarningMsg);
} // namespace

QString I3IpcEvent::type() const { return I3IpcEvent::eventToString(this->mCode); }
QString I3IpcEvent::data() const { return QString::fromUtf8(this->mData.toJson()); }

EventCode I3IpcEvent::intToEvent(quint32 raw) {
	if ((EventCode::Workspace <= raw && raw <= EventCode::Trails)
	    || (EventCode::RunCommand <= raw && raw <= EventCode::GetSpaces))
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
	case EventCode::GetMarks: return "get_marks"; break;
	case EventCode::GetVersion: return "get_version"; break;
	case EventCode::GetBindingModes: return "get_binding_modes"; break;
	case EventCode::GetBindingState: return "get_binding_state"; break;
	case EventCode::GetInputs: return "get_inputs"; break;
	case EventCode::GetScroller: return "get_scroller"; break;
	case EventCode::GetTrails: return "get_trails"; break;
	case EventCode::GetSpaces: return "get_spaces"; break;

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
	case EventCode::Scroller: return "scroller"; break;
	case EventCode::Trails: return "trails"; break;

	default: return "unknown"; break;
	}
}

I3Ipc::I3Ipc(const QList<QString>& events): mEvents(events) {
	auto sock = qEnvironmentVariable("SCROLLSOCK");

	if (sock.isEmpty()) {
		qCWarning(logI3Ipc) << "$SCROLLSOCK is unset. Trying $SWAYSOCK.";

		sock = qEnvironmentVariable("SWAYSOCK");

		if (sock.isEmpty()) {
			qCWarning(logI3Ipc) << "$SCROLLSOCK and $SWAYSOCK are unset. Trying $I3SOCK.";

			sock = qEnvironmentVariable("I3SOCK");

			if (sock.isEmpty()) {
				qCWarning(logI3Ipc) << "$SCROLLSOCK, $SWAYSOCK and $I3SOCK are unset. Cannot connect to socket.";
				return;
			} else {
				this->mCompositor = "i3";
			}
		} else {
			this->mCompositor = "sway";
		}
	} else {
		this->mCompositor = "scroll";
		mEvents.append("scroller");
		mEvents.append("trails");
	}

	this->mSocketPath = sock;

	// clang-format off
	QObject::connect(&this->liveEventSocket, &QLocalSocket::errorOccurred, this, &I3Ipc::eventSocketError);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::stateChanged, this, &I3Ipc::eventSocketStateChanged);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::readyRead, this, &I3Ipc::eventSocketReady);
	QObject::connect(&this->liveEventSocket, &QLocalSocket::connected, this, &I3Ipc::subscribe);
	// clang-format on

	this->liveEventSocketDs.setDevice(&this->liveEventSocket);
	this->liveEventSocketDs.setByteOrder(static_cast<QDataStream::ByteOrder>(QSysInfo::ByteOrder));
}

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

void I3Ipc::subscribe() {
	auto jsonArray = QJsonArray::fromStringList(this->mEvents);
	auto jsonDoc = QJsonDocument(jsonArray);
	auto payload = jsonDoc.toJson(QJsonDocument::Compact);
	auto message = I3Ipc::buildRequestMessage(EventCode::Subscribe, payload);

	this->makeRequest(message);
}

void I3Ipc::eventSocketReady() {
	for (auto& [type, data]: this->parseResponse()) {
		this->event.mCode = type;
		this->event.mData = data;

		emit this->rawEvent(&this->event);
	}
}

void I3Ipc::connect() { this->liveEventSocket.connectToServer(this->mSocketPath); }

void I3Ipc::reconnectIPC() {
	qCWarning(logI3Ipc) << "Fatal IPC error occured, recreating connection";
	this->liveEventSocket.disconnectFromServer();
	this->connect();
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

QString I3Ipc::compositor() const { return this->mCompositor; }

} // namespace qs::i3::ipc
