#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"

namespace qs::network {

///! A tracked access point on the network
class NetworkAccessPoint: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WirelessNetwork can only be acquired through Network");
	// clang-format off
	/// The service set identifier of the access point.
	Q_PROPERTY(QString ssid READ default NOTIFY ssidChanged BINDABLE bindableSsid);
	// The current signal quality of the access point, in percent.
	Q_PROPERTY(quint8 signal READ default NOTIFY signalChanged BINDABLE bindableSignal);
	//clang-format on

signals:
	void ssidChanged();
	void signalChanged();

public slots:
	void setSsid(const QString& ssid);
	void setSignal(quint8 signal);

public:
	explicit NetworkAccessPoint(QObject* parent = nullptr);

	[[nodiscard]] QBindable<QString> bindableSsid() const { return &this->bSsid; };
	[[nodiscard]] QBindable<quint8> bindableSignal() const { return &this->bSignal; };

private:
	Q_OBJECT_BINDABLE_PROPERTY(NetworkAccessPoint, QString, bSsid, &NetworkAccessPoint::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkAccessPoint, quint8, bSignal, &NetworkAccessPoint::signalChanged);
};

///! Type of network device.
class NetworkDeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		///! A generic device.
		Other = 0,
		///! An 802.11 Wi-Fi device.
		Wireless = 1,
		///! A wired ethernet device.
		Ethernet = 2
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkDeviceType::Enum type);
};

///! State of a network device.
class NetworkDeviceState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// The device state is unknown.
		Unknown = 0,
		/// The device is not connected.
		Disconnected = 1,
		/// The device is connected.
		Connected = 2,
		/// The device is disconnecting.
		Disconnecting = 3,
		/// The device is connecting.
		Connecting = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkDeviceState::Enum state);
};

///! A tracked network device.
class NetworkDevice: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("NetworkDevices can only be acquired through Network");

	// clang-format off
	/// The name of the device's interface.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// The hardware address of the device's interface in the XX:XX:XX:XX:XX:XX format.
	Q_PROPERTY(QString address READ default NOTIFY addressChanged BINDABLE bindableAddress);
	/// Connection state of the device.
	Q_PROPERTY(NetworkDeviceState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// Type of device.
	Q_PROPERTY(NetworkDeviceType::Enum type READ type CONSTANT);
	// clang-format on

signals:
	void nameChanged();
	void addressChanged();
	void stateChanged();

	void signalDisconnect();

public slots:
	void setName(const QString& name);
	void setAddress(const QString& address);
	void setState(NetworkDeviceState::Enum state);

public:
	explicit NetworkDevice(QObject* parent = nullptr);

	/// Disconnects the device and prevents it from automatically activating further connections.
	Q_INVOKABLE void disconnect();
	[[nodiscard]] virtual NetworkDeviceType::Enum type() const { return NetworkDeviceType::Other; };

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; };
	[[nodiscard]] QBindable<QString> bindableAddress() const { return &this->bAddress; };
	[[nodiscard]] QBindable<NetworkDeviceState::Enum> bindableState() const { return &this->bState; };

private:
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bName, &NetworkDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bAddress, &NetworkDevice::addressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(
	    NetworkDevice,
	    NetworkDeviceState::Enum,
	    bState,
	    &NetworkDevice::stateChanged
	);
};

///! Wireless variant of a tracked network device.
class WirelessNetworkDevice: public NetworkDevice {
	Q_OBJECT;

	// clang-format off
	/// The timestamp (in CLOCK_BOOTTIME milliseconds) for the last finished network scan.
	Q_PROPERTY(qint64 lastScan READ default NOTIFY lastScanChanged BINDABLE bindableLastScan);
	/// True if the wireless device is currently scanning for available wifi networks.
	Q_PROPERTY(bool scanning READ default NOTIFY scanningChanged BINDABLE bindableScanning);
	/// A list of all available access points
	Q_PROPERTY(UntypedObjectModel* accessPoints READ accessPoints CONSTANT); 
	QSDOC_TYPE_OVERRIDE(ObjectModel<NetworkAccessPoint>*)
	//clang-format on

signals:
	void signalScan();

	void lastScanChanged();
	void scanningChanged();

public slots:
	void scanComplete(qint64 lastScan);
	void addAccessPoint(NetworkAccessPoint* ap);
	void removeAccessPoint(NetworkAccessPoint* ap);

public:
	explicit WirelessNetworkDevice(QObject* parent = nullptr);
	[[nodiscard]] NetworkDeviceType::Enum type() const override { return NetworkDeviceType::Wireless; };

	/// Request the wireless device to scan for available WiFi networks.
	Q_INVOKABLE void scan();

	[[nodiscard]] QBindable<bool> bindableScanning() { return &this->bScanning; };
	[[nodiscard]] QBindable<qint64> bindableLastScan() { return &this->bLastScan; };

	UntypedObjectModel* accessPoints() { return &this->mAccessPoints; };

private:
	ObjectModel<NetworkAccessPoint> mAccessPoints{this};
	Q_OBJECT_BINDABLE_PROPERTY(WirelessNetworkDevice, bool, bScanning, &WirelessNetworkDevice::scanningChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WirelessNetworkDevice, qint64, bLastScan, &WirelessNetworkDevice::lastScanChanged);
};

// -- Network --
class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

///! Network manager
/// Provides access to network devices.
class Network: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Network);
	QML_SINGLETON;

	// clang-format off
	/// The default wifi device. Usually there is only one. This defaults to the first wifi device registered.
	// Q_PROPERTY(WirelessNetworkDevice* defaultWifiDevice READ defaultWifiDevice CONSTANT);
	/// A list of all network devices.
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::NetworkDevice>*);
	// clang-format on

public slots:
	void addDevice(NetworkDevice* device);
	void removeDevice(NetworkDevice* device);

public:
	explicit Network(QObject* parent = nullptr);
	[[nodiscard]] UntypedObjectModel* devices() { return backend ? &this->mDevices : nullptr; };

private:
	ObjectModel<NetworkDevice> mDevices {this};
	class NetworkBackend* backend = nullptr;
};

} // namespace qs::network
