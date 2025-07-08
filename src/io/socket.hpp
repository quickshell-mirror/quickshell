#pragma once

#include <qcontainerfwd.h>
#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "../core/reload.hpp"
#include "datastream.hpp"

QS_DECLARE_LOGGING_CATEGORY(logSocket);

///! Unix socket listener.
class Socket: public DataStream {
	Q_OBJECT;
	/// Returns if the socket is currently connected.
	///
	/// Writing to this property will set the target connection state and will not
	/// update the property immediately. Setting the property to false will begin disconnecting
	/// the socket, and setting it to true will begin connecting the socket if path is not empty.
	Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectionStateChanged);
	/// The path to connect this socket to when @@connected is set to true.
	///
	/// Changing this property will have no effect while the connection is active.
	Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged);
	QML_ELEMENT;

public:
	explicit Socket(QObject* parent = nullptr): DataStream(parent) {}

	/// Write data to the socket. Does nothing if not connected.
	///
	/// Remember to call flush after your last write.
	Q_INVOKABLE void write(const QString& data);

	/// Flush any queued writes to the socket.
	Q_INVOKABLE void flush();

	// takes ownership
	void setSocket(QLocalSocket* socket);

	[[nodiscard]] bool isConnected() const;
	void setConnected(bool connected);

	[[nodiscard]] QString path() const;
	void setPath(QString path);

signals:
	/// This signal is sent whenever a socket error is encountered.
	void error(QLocalSocket::LocalSocketError error);

	void connectionStateChanged();
	void pathChanged();

protected:
	[[nodiscard]] QIODevice* ioDevice() const override;

private slots:
	void onSocketConnected();
	void onSocketDisconnected();
	void onSocketError(QLocalSocket::LocalSocketError error);

private:
	void connectPathSocket();

	QLocalSocket* socket = nullptr;
	bool connected = false;
	bool disconnecting = false;
	bool targetConnected = false;
	QString mPath;
};

///! Unix socket server.
/// #### Example
/// ```qml
/// SocketServer {
///   active: true
///   path: "/path/too/socket.sock"
///   handler: Socket {
///     onConnectedChanged: {
///       console.log(connected ? "new connection!" : "connection dropped!")
///     }
///     parser: SplitParser {
///       onRead: message => console.log(`read message from socket: ${message}`)
///     }
///   }
/// }
/// ```
class SocketServer: public Reloadable {
	Q_OBJECT;
	/// If the socket server is currently active. Defaults to false.
	///
	/// Setting this to false will destory all active connections and delete
	/// the socket file on disk.
	///
	/// If path is empty setting this property will have no effect.
	Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeStatusChanged);
	/// The path to create the socket server at.
	///
	/// Setting this property while the server is active will have no effect.
	Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged);
	/// Connection handler component. Must creeate a @@Socket.
	///
	/// The created socket should not set @@connected or @@path or the incoming
	/// socket connection will be dropped (they will be set by the socket server.)
	/// Setting `connected` to false on the created socket after connection will
	/// close and delete it.
	Q_PROPERTY(QQmlComponent* handler READ handler WRITE setHandler NOTIFY handlerChanged);
	QML_ELEMENT;

public:
	explicit SocketServer(QObject* parent = nullptr): Reloadable(parent) {}
	~SocketServer() override;
	Q_DISABLE_COPY_MOVE(SocketServer);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] bool isActive() const;
	void setActive(bool active);

	[[nodiscard]] QString path() const;
	void setPath(QString path);

	[[nodiscard]] QQmlComponent* handler() const;
	void setHandler(QQmlComponent* handler);

signals:
	void activeStatusChanged();
	void pathChanged();
	void handlerChanged();

private slots:
	void onNewConnection();

private:
	bool isActivatable();
	void enableServer();
	void disableServer();

	QLocalServer* server = nullptr;
	QQmlComponent* mHandler = nullptr;
	QList<Socket*> mSockets;
	bool activeTarget = false;
	bool postReload = false;
	QString mPath;
	QString activePath;
};
