#include "ipc.hpp"
#include <functional>
#include <variant>

#include <qbuffer.h>
#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>

#include "../core/generation.hpp"
#include "../core/logcat.hpp"
#include "../core/paths.hpp"
#include "ipccommand.hpp"

namespace qs::ipc {

QS_LOGGING_CATEGORY(logIpc, "quickshell.ipc", QtWarningMsg);

IpcServer::IpcServer(const QString& path) {
	QObject::connect(&this->server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);

	QLocalServer::removeServer(path);

	if (!this->server.listen(path)) {
		qCCritical(logIpc) << "Failed to start IPC server on path" << path;
		return;
	}

	qCInfo(logIpc) << "Started IPC server on path" << path;
}

void IpcServer::start() {
	if (auto* run = QsPaths::instance()->instanceRunDir()) {
		auto path = run->filePath("ipc.sock");
		new IpcServer(path);
	} else {
		qCCritical(logIpc
		) << "Could not start IPC server as the instance runtime path could not be created.";
	}
}

void IpcServer::onNewConnection() {
	while (auto* connection = this->server.nextPendingConnection()) {
		new IpcServerConnection(connection, this);
	}
}

IpcServerConnection::IpcServerConnection(QLocalSocket* socket, IpcServer* server)
    : QObject(server)
    , socket(socket) {
	socket->setParent(this);
	this->stream.setDevice(socket);
	QObject::connect(socket, &QLocalSocket::disconnected, this, &IpcServerConnection::onDisconnected);
	QObject::connect(socket, &QLocalSocket::readyRead, this, &IpcServerConnection::onReadyRead);

	qCInfo(logIpc) << "New IPC connection" << this;
}

void IpcServerConnection::onDisconnected() {
	qCInfo(logIpc) << "IPC connection disconnected" << this;
}

void IpcServerConnection::onReadyRead() {
	this->stream.startTransaction();

	this->stream.startTransaction();
	IpcCommand command;
	this->stream >> command;
	if (!this->stream.commitTransaction()) return;

	std::visit(
	    [this]<typename Command>(Command& command) {
		    if constexpr (std::is_same_v<std::monostate, Command>) {
			    qCCritical(logIpc) << "Received invalid IPC command from" << this;
			    this->socket->disconnectFromServer();
		    } else {
			    command.exec(this);
		    }
	    },
	    command
	);

	if (!this->stream.commitTransaction()) return;
}

IpcClient::IpcClient(const QString& path) {
	QObject::connect(&this->socket, &QLocalSocket::connected, this, &IpcClient::connected);
	QObject::connect(&this->socket, &QLocalSocket::disconnected, this, &IpcClient::disconnected);
	QObject::connect(&this->socket, &QLocalSocket::errorOccurred, this, &IpcClient::onError);

	this->socket.connectToServer(path);
	this->stream.setDevice(&this->socket);
}

bool IpcClient::isConnected() const { return this->socket.isValid(); }

void IpcClient::waitForConnected() { this->socket.waitForConnected(); }
void IpcClient::waitForDisconnected() { this->socket.waitForDisconnected(); }

void IpcClient::kill() { this->sendMessage(IpcCommand(IpcKillCommand())); }

void IpcClient::onError(QLocalSocket::LocalSocketError error) {
	qCCritical(logIpc) << "Socket Error" << error;
}

int IpcClient::connect(const QString& id, const std::function<void(IpcClient& client)>& callback) {
	auto path = QsPaths::ipcPath(id);
	auto client = IpcClient(path);
	qCDebug(logIpc) << "Connecting to instance" << id << "at" << path;

	client.waitForConnected();
	if (!client.isConnected()) return -1;
	qCDebug(logIpc) << "Connected.";

	callback(client);
	return 0;
}

void IpcKillCommand::exec(IpcServerConnection* /*unused*/) {
	qInfo() << "Exiting due to IPC request.";
	EngineGeneration::currentGeneration()->quit();
}

} // namespace qs::ipc
