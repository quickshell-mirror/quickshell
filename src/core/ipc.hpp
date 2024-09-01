#pragma once

#include <functional>

#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::ipc {

enum class IpcCommand : quint8 {
	Unknown = 0,
	Kill,
};

class IpcServer: public QObject {
	Q_OBJECT;

public:
	explicit IpcServer(const QString& path);

	static void start();

private slots:
	void onNewConnection();

private:
	QLocalServer server;
};

class IpcServerConnection: public QObject {
	Q_OBJECT;

public:
	explicit IpcServerConnection(QLocalSocket* socket, IpcServer* server);

private slots:
	void onDisconnected();
	void onReadyRead();

private:
	QLocalSocket* socket;
	QDataStream stream;
};

class IpcClient: public QObject {
	Q_OBJECT;

public:
	explicit IpcClient(const QString& path);

	[[nodiscard]] bool isConnected() const;
	void waitForConnected();
	void waitForDisconnected();

	void kill();

	[[nodiscard]] static int
	connect(const QString& id, const std::function<void(IpcClient& client)>& callback);

signals:
	void connected();
	void disconnected();

private slots:
	static void onError(QLocalSocket::LocalSocketError error);

private:
	QLocalSocket socket;
	QDataStream stream;
};

} // namespace qs::ipc
