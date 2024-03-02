#include "socket.hpp"
#include <utility>

#include <qlocalsocket.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "datastream.hpp"

void Socket::setSocket(QLocalSocket* socket) {
	if (this->socket != nullptr) this->socket->deleteLater();

	this->socket = socket;
	socket->setParent(this);

	if (socket != nullptr) {
		// clang-format off
		QObject::connect(this->socket, &QLocalSocket::connected, this, &Socket::onSocketConnected);
		QObject::connect(this->socket, &QLocalSocket::disconnected, this, &Socket::onSocketDisconnected);
		QObject::connect(this->socket, &QLocalSocket::errorOccurred, this, &Socket::error);
		QObject::connect(this->socket, &QLocalSocket::readyRead, this, &DataStream::onBytesAvailable);
		// clang-format on

		if (socket->isOpen()) this->onSocketConnected();
	}
}

QString Socket::path() const { return this->mPath; }

void Socket::setPath(QString path) {
	if ((this->connected && !this->disconnecting) || path == this->mPath) return;
	this->mPath = std::move(path);
	emit this->pathChanged();

	if (this->targetConnected && this->socket == nullptr) this->connectPathSocket();
}

void Socket::onSocketConnected() {
	this->connected = true;
	this->targetConnected = false;
	this->disconnecting = false;
	emit this->connectionStateChanged();
}

void Socket::onSocketDisconnected() {
	this->connected = false;
	this->disconnecting = false;
	this->socket->deleteLater();
	this->socket = nullptr;
	emit this->connectionStateChanged();

	if (this->targetConnected) this->connectPathSocket();
}

bool Socket::isConnected() const { return this->connected; }

void Socket::setConnected(bool connected) {
	this->targetConnected = connected;

	if (!connected) {
		if (this->socket != nullptr && !this->disconnecting) {
			this->disconnecting = true;
			this->socket->disconnectFromServer();
		}
	} else if (this->socket == nullptr) this->connectPathSocket();
}

QIODevice* Socket::ioDevice() const { return this->socket; }

void Socket::connectPathSocket() {
	if (!this->mPath.isEmpty()) {
		auto* socket = new QLocalSocket();
		socket->setServerName(this->mPath);
		this->setSocket(socket);
		this->socket->connectToServer(QIODevice::ReadWrite);
	}
}
