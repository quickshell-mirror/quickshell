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
#include "../dbus/properties.hpp"

namespace qs::network {

class Device: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(QString address READ default NOTIFY addressChanged BINDABLE bindableAddress);
	Q_PROPERTY(bool powered READ default NOTIFY poweredChanged BINDABLE bindablePowered);

	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");

public:
	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; };
	[[nodiscard]] QBindable<QString> bindableAddress() const { return &this->bAddress; };
	[[nodiscard]] QBindable<bool> bindablePowered() const { return &this->bPowered; };

signals:
	void nameChanged();
	void addressChanged();
	void poweredChanged();

protected:
	explicit Device(QObject* parent = nullptr);
	Q_OBJECT_BINDABLE_PROPERTY(Device, QString, bName, &Device::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Device, bool, bPowered, &Device::poweredChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Device, QString, bAddress, &Device::addressChanged);
	QS_DBUS_BINDABLE_PROPERTY_GROUP(Device, deviceProperties);
};

// class WirelessDevice;

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;
	virtual Device* wireless() = 0;
	virtual UntypedObjectModel* allDevices() = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

class Network: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Network);
	QML_SINGLETON;

	Q_PROPERTY(Device* wireless READ wireless CONSTANT);
	Q_PROPERTY(UntypedObjectModel* allDevices READ allDevices CONSTANT);

public:
	explicit Network(QObject* parent = nullptr);
	[[nodiscard]] Device* wireless() { return backend ? backend->wireless() : nullptr; }
	[[nodiscard]] UntypedObjectModel* allDevices() { return backend ? backend->allDevices() : nullptr; }

private:
	void findBackend();
	class NetworkBackend* backend = nullptr;
};

} // namespace qs::network
