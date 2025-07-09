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

#include "../../core/model.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_service.h"
#include "device.hpp"

namespace qs::service::networkmanager {

class NetworkManagerState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Asleep = 10,
		Disconnected = 20,
		Disconnecting = 30,
		Connecting = 40,
		ConnectedLocal = 50,
		ConnectedSite = 60,
		ConnectedGlobal = 70,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(qs::service::networkmanager::NetworkManagerState::Enum state);
};

} // namespace qs::service::networkmanager

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::networkmanager::NetworkManagerState::Enum> {
	using Wire = quint32;
	using Data = qs::service::networkmanager::NetworkManagerState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::service::networkmanager {

class NetworkManager: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] NMDevice* wifiDevice();
	[[nodiscard]] ObjectModel<NMDevice>* devices();
	[[nodiscard]] QBindable<NetworkManagerState::Enum> bindableState() const {
		return &this->bState;
	};

	static NetworkManager* instance();

signals:
	void stateChanged();

private slots:
	void onDeviceAdded(const QDBusObjectPath& path);
	void onDeviceRemoved(const QDBusObjectPath& path);

private:
	explicit NetworkManager();

	void init();
	void registerDevice(const QString& path);
	void createDevice(const QString& path);
	void registerDevices();

	QHash<QString, NMDevice*> mDeviceHash;
	ObjectModel<NMDevice> mDevices {this};
	NMDevice* mWifiDevice = nullptr;

	Q_OBJECT_BINDABLE_PROPERTY(
	    NetworkManager,
	    NetworkManagerState::Enum,
	    bState,
	    &NetworkManager::stateChanged
	);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, serviceProperties);
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pState, bState, serviceProperties, "State");

	DBusNetworkManagerService* service = nullptr;
};

///! Provides access to the NetworkManager service.
/// An interface to the [NetworkManager daemon], which can be used to
/// view and configure network interfaces and connections.
///
/// > [!NOTE] The NetworkManager daemon must be installed to use this service.
///
/// [NetworkManager daemon]: https://networkmanager.dev
class NetworkManagerQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(NetworkManager);
	QML_SINGLETON;
	Q_PROPERTY(qs::service::networkmanager::NMDevice* wifiDevice READ wifiDevice CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::networkmanager::NetworkManagerService>*);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	// clang-format off
	Q_PROPERTY(NetworkManagerState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	// clang-format on

public:
	explicit NetworkManagerQml(QObject* parent = nullptr);
	[[nodiscard]] static NMDevice* wifiDevice() { return NetworkManager::instance()->wifiDevice(); }

	[[nodiscard]] static ObjectModel<NMDevice>* devices() {
		return NetworkManager::instance()->devices();
	}

	[[nodiscard]] static QBindable<NetworkManagerState::Enum> bindableState() {
		return NetworkManager::instance()->bindableState();
	}

signals:
	void stateChanged();
};

} // namespace qs::service::networkmanager
