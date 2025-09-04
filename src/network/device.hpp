#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "nm/enums.hpp"

namespace qs::network {

///! Connection state.
class NetworkConnectionState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Connecting = 1,
		Connected = 2,
		Disconnecting = 3,
		Disconnected = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkConnectionState::Enum state);
};

///! A Network device.
class NetworkDevice: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");

	/// The name of the device's control interface.
	Q_PROPERTY(QString name READ name NOTIFY nameChanged);
	/// The hardware address of the device in the XX:XX:XX:XX:XX:XX format.
	Q_PROPERTY(QString address READ address NOTIFY addressChanged);
	/// Connection state of the device.
	Q_PROPERTY(NetworkConnectionState::Enum state READ state NOTIFY stateChanged);
	/// A more specific device state when the backend is NetworkManager.
	Q_PROPERTY(NMDeviceState::Enum nmState READ nmState NOTIFY nmStateChanged);

signals:
	void nameChanged();
	void addressChanged();
	void stateChanged();
	void nmStateChanged();
	void requestDisconnect();

public slots:
	void setName(const QString& name);
	void setAddress(const QString& address);
	void setState(NetworkConnectionState::Enum state);
	void setNmState(NMDeviceState::Enum state);

public:
	explicit NetworkDevice(QObject* parent = nullptr);

	[[nodiscard]] QString name() const { return this->mName; };
	[[nodiscard]] QString address() const { return this->mAddress; };
	[[nodiscard]] NetworkConnectionState::Enum state() const { return this->mState; };
	[[nodiscard]] NMDeviceState::Enum nmState() const { return this->mNmState; };

	/// Disconnects the device and prevents it from automatically activating further connections.
	Q_INVOKABLE void disconnect();

private:
	QString mName;
	QString mAddress;
	NetworkConnectionState::Enum mState = NetworkConnectionState::Unknown;
	NMDeviceState::Enum mNmState = NMDeviceState::Unknown;
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NetworkDevice* device);
