#pragma once

#include <qlocalsocket.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "datastream.hpp"

///! Unix socket listener.
class Socket: public DataStream {
	Q_OBJECT;
	/// Returns if the socket is currently connected.
	///
	/// Writing to this property will set the target connection state and will not
	/// update the property immediately. Setting the property to false will begin disconnecting
	/// the socket, and setting it to true will begin connecting the socket if path is not empty.
	Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectionStateChanged);
	/// The path to connect this socket to when `connected` is set to true.
	///
	/// Changing this property will have no effect while the connection is active.
	Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged);
	QML_ELEMENT;

public:
	explicit Socket(QObject* parent = nullptr): DataStream(parent) {}

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

private:
	void connectPathSocket();

	QLocalSocket* socket = nullptr;
	bool connected = false;
	bool disconnecting = false;
	bool targetConnected = false;
	QString mPath;
};
