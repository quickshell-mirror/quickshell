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

// -- Device --

class DeviceState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Disconnected = 10,
		Connecting = 20,
		Connected = 30,
		Disconnecting = 40,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(DeviceState::Enum state);
};

class Device: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");

	// clang-format off
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(QString address READ default NOTIFY addressChanged BINDABLE bindableAddress);
	Q_PROPERTY(DeviceState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	// clang-format on

signals:
	void nameChanged();
	void addressChanged();
	void stateChanged();

public slots:
	void setName(const QString& name);
	void setAddress(const QString& address);
	void setState(DeviceState::Enum state);

public:
	explicit Device(QObject* parent = nullptr);
	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; };
	[[nodiscard]] QBindable<QString> bindableAddress() const { return &this->bAddress; };
	[[nodiscard]] QBindable<DeviceState::Enum> bindableState() const { return &this->bState; };

private:
	Q_OBJECT_BINDABLE_PROPERTY(Device, QString, bName, &Device::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Device, QString, bAddress, &Device::addressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Device, DeviceState::Enum, bState, &Device::stateChanged);
};

// -- Wireless Device --
class WirelessDevice: public Device {
	Q_OBJECT;

	// clang-format off
	Q_PROPERTY(qint64 lastScan READ default NOTIFY lastScanChanged BINDABLE bindableLastScan);
	//clang-format on

signals:
	void lastScanChanged();

public slots:
	void setLastScan(qint64 lastScan);

public:
	explicit WirelessDevice(QObject* parent = nullptr);

	// Q_INVOKABLE void disconnect();
	// Q_INVOKABLE void scan();

	[[nodiscard]] QBindable<qint64> bindableLastScan() { return &this->bLastScan; };

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WirelessDevice, qint64, bLastScan, &WirelessDevice::lastScanChanged);
	// clang-format on
};

// -- Network --

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;
	virtual UntypedObjectModel* devices() = 0;
	virtual WirelessDevice* defaultWifiDevice() = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

class Network: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Network);
	QML_SINGLETON;

	Q_PROPERTY(WirelessDevice* defaultWifiDevice READ defaultWifiDevice CONSTANT);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);

public:
	explicit Network(QObject* parent = nullptr);
	[[nodiscard]] UntypedObjectModel* devices() { return backend ? backend->devices() : nullptr; };
	[[nodiscard]] WirelessDevice* defaultWifiDevice() {
		return backend ? backend->defaultWifiDevice() : nullptr;
	};

private:
	class NetworkBackend* backend = nullptr;
};

} // namespace qs::network
