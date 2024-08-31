#include "ipc.hpp"
#include <functional>

#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>

#include "generation.hpp"
#include "paths.hpp"

namespace qs::ipc {

Q_LOGGING_CATEGORY(logIpc, "quickshell.ipc", QtWarningMsg);

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
	auto command = IpcCommand::Unknown;
	this->stream >> command;
	if (!this->stream.commitTransaction()) return;

	switch (command) {
	case IpcCommand::Kill:
		qInfo() << "Exiting due to IPC request.";
		EngineGeneration::currentGeneration()->quit();
		break;
	default:
		qCCritical(logIpc) << "Received invalid IPC command from" << this;
		this->socket->disconnectFromServer();
		break;
	}

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

void IpcClient::kill() {
	qCDebug(logIpc) << "Sending kill command...";
	this->stream << IpcCommand::Kill;
	this->socket.flush();
}

void IpcClient::onError(QLocalSocket::LocalSocketError error) {
	qCCritical(logIpc) << "Socket Error" << error;
}

bool IpcClient::connect(const QString& id, const std::function<void(IpcClient& client)>& callback) {
	auto path = QsPaths::ipcPath(id);
	auto client = IpcClient(path);
	qCDebug(logIpc) << "Connecting to instance" << id << "at" << path;

	client.waitForConnected();
	if (!client.isConnected()) return false;
	qCDebug(logIpc) << "Connected.";

	callback(client);
	return true;
}
} // namespace qs::ipc
