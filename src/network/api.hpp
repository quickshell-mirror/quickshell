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

class Device: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");

	// clang-format off
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(QString address READ default NOTIFY addressChanged BINDABLE bindableAddress);
	// clang-format on

signals:
	void nameChanged();
	void addressChanged();

public slots:
	void setName(const QString& name);
	void setAddress(const QString& address);

public:
	explicit Device(QObject* parent = nullptr);
	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; };
	[[nodiscard]] QBindable<QString> bindableAddress() const { return &this->bAddress; };

private:
	Q_OBJECT_BINDABLE_PROPERTY(Device, QString, bName, &Device::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Device, QString, bAddress, &Device::addressChanged);
};

// -- Wireless Device --

class WirelessState: public QObject {
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
};

class WirelessDevice: public Device {
	Q_OBJECT;

	// clang-format off
	Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged);
	Q_PROPERTY(WirelessState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	//clang-format on

signals:
	void poweredChanged();
	void stateChanged();

private slots:
	void setState(WirelessState::Enum state);

public:
	explicit WirelessDevice(QObject* parent = nullptr);

	Q_INVOKABLE virtual void disconnect() = 0;
	Q_INVOKABLE virtual void scan() = 0;

	[[nodiscard]] bool powered() const { return this->bPowered; };
	virtual void setPowered(bool powered) = 0;

	[[nodiscard]] QBindable<bool> bindablePowered() { return &this->bPowered; };
	[[nodiscard]] QBindable<WirelessState::Enum> bindableState() const { return &this->bState; };

private:
	Q_OBJECT_BINDABLE_PROPERTY(WirelessDevice, bool, bPowered, &WirelessDevice::poweredChanged);
	Q_OBJECT_BINDABLE_PROPERTY(
	    WirelessDevice,
	    WirelessState::Enum,
	    bState,
	    &WirelessDevice::stateChanged
	);
};

// -- Network --

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;
	// virtual WirelessDevice* wifiDevice() = 0;
	virtual UntypedObjectModel* devices() = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

class Network: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Network);
	QML_SINGLETON;

	// Q_PROPERTY(WirelessDevice* wifi READ wifiDevice CONSTANT);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);

public:
	explicit Network(QObject* parent = nullptr);
	// [[nodiscard]] WirelessDevice* wifi() { return backend ? backend->wifiDevice() : nullptr; }
	[[nodiscard]] UntypedObjectModel* devices() { return backend ? backend->devices() : nullptr; }

private:
	class NetworkBackend* backend = nullptr;
};

} // namespace qs::network
