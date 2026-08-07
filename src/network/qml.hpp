#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"
#include "../core/model.hpp"
#include "device.hpp"
#include "enums.hpp"

namespace qs::network {

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

class Networking: public QObject {
	Q_OBJECT;

public:
	static Networking* instance();

	void checkConnectivity();

	[[nodiscard]] ObjectModel<NetworkDevice>* devices() { return &this->mDevices; }
	[[nodiscard]] NetworkBackendType::Enum backend() const { return this->mBackendType; }
	QBindable<bool> bindableWifiEnabled() { return &this->bWifiEnabled; }
	[[nodiscard]] bool wifiEnabled() const { return this->bWifiEnabled; }
	void setWifiEnabled(bool enabled);
	QBindable<bool> bindableWifiHardwareEnabled() { return &this->bWifiHardwareEnabled; }
	QBindable<bool> bindableCanCheckConnectivity() { return &this->bCanCheckConnectivity; }
	QBindable<bool> bindableConnectivityCheckEnabled() { return &this->bConnectivityCheckEnabled; }
	[[nodiscard]] bool connectivityCheckEnabled() const { return this->bConnectivityCheckEnabled; }
	void setConnectivityCheckEnabled(bool enabled);
	QBindable<NetworkConnectivity::Enum> bindableConnectivity() { return &this->bConnectivity; }

signals:
	void requestSetWifiEnabled(bool enabled);
	void requestSetConnectivityCheckEnabled(bool enabled);
	void requestCheckConnectivity();

	void wifiEnabledChanged();
	void wifiHardwareEnabledChanged();
	void canCheckConnectivityChanged();
	void connectivityCheckEnabledChanged();
	void connectivityChanged();

private slots:
	void deviceAdded(NetworkDevice* dev);
	void deviceRemoved(NetworkDevice* dev);

private:
	explicit Networking(QObject* parent = nullptr);

	ObjectModel<NetworkDevice> mDevices {this};
	NetworkBackend* mBackend = nullptr;
	NetworkBackendType::Enum mBackendType = NetworkBackendType::None;
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Networking, bool, bWifiEnabled, &Networking::wifiEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Networking, bool, bWifiHardwareEnabled, &Networking::wifiHardwareEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Networking, bool, bCanCheckConnectivity, &Networking::canCheckConnectivityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Networking, bool, bConnectivityCheckEnabled, &Networking::connectivityCheckEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Networking, NetworkConnectivity::Enum, bConnectivity, &Networking::connectivityChanged);
	// clang-format on
};

///! The Network service.
/// An interface to a network backend (currently only NetworkManager),
/// which can be used to view, configure, and connect to various networks.
class NetworkingQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Networking);
	QML_SINGLETON;
	// clang-format off
	/// A list of all network devices. Networks are exposed through their respective devices.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::NetworkDevice>*);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	/// The backend being used to power the Network service.
	Q_PROPERTY(qs::network::NetworkBackendType::Enum backend READ backend CONSTANT);
	/// Switch for the rfkill software block of all wireless devices.
	Q_PROPERTY(bool wifiEnabled READ wifiEnabled WRITE setWifiEnabled NOTIFY wifiEnabledChanged);
	/// State of the rfkill hardware block of all wireless devices.
	Q_PROPERTY(bool wifiHardwareEnabled READ default NOTIFY wifiHardwareEnabledChanged BINDABLE bindableWifiHardwareEnabled);
	/// True if the @@backend supports connectivity checks.
	Q_PROPERTY(bool canCheckConnectivity READ default NOTIFY canCheckConnectivityChanged BINDABLE bindableCanCheckConnectivity);
	/// True if connectivity checking is enabled.
	Q_PROPERTY(bool connectivityCheckEnabled READ connectivityCheckEnabled WRITE setConnectivityCheckEnabled NOTIFY connectivityCheckEnabledChanged);
	/// The result of the last connectivity check.
	///
  /// Connectivity checks may require additional configuration depending on your distro.
  ///
	/// > [!NOTE] This property can be used to determine if network access is restricted
  /// > or gated behind a captive portal.
	/// >
  /// > If checking for captive portals, @@checkConnectivity() should be called after
  /// > the portal is dismissed to update this property.
	Q_PROPERTY(qs::network::NetworkConnectivity::Enum connectivity READ default NOTIFY connectivityChanged BINDABLE bindableConnectivity);
	// clang-format on

public:
	explicit NetworkingQml(QObject* parent = nullptr);

	/// Re-check the network connectivity state immediately.
	/// > [!NOTE] This should be invoked after a user dismisses a web browser that was opened to authenticate via a captive portal.
	Q_INVOKABLE static void checkConnectivity();

	[[nodiscard]] static ObjectModel<NetworkDevice>* devices() {
		return Networking::instance()->devices();
	}
	[[nodiscard]] static NetworkBackendType::Enum backend() {
		return Networking::instance()->backend();
	}
	[[nodiscard]] static bool wifiEnabled() { return Networking::instance()->wifiEnabled(); }
	static void setWifiEnabled(bool enabled) { Networking::instance()->setWifiEnabled(enabled); }
	[[nodiscard]] static QBindable<bool> bindableWifiHardwareEnabled() {
		return Networking::instance()->bindableWifiHardwareEnabled();
	}
	[[nodiscard]] static QBindable<bool> bindableWifiEnabled() {
		return Networking::instance()->bindableWifiEnabled();
	}
	[[nodiscard]] static QBindable<bool> bindableCanCheckConnectivity() {
		return Networking::instance()->bindableCanCheckConnectivity();
	}
	[[nodiscard]] static bool connectivityCheckEnabled() {
		return Networking::instance()->connectivityCheckEnabled();
	}
	static void setConnectivityCheckEnabled(bool enabled) {
		Networking::instance()->setConnectivityCheckEnabled(enabled);
	}
	[[nodiscard]] static QBindable<NetworkConnectivity::Enum> bindableConnectivity() {
		return Networking::instance()->bindableConnectivity();
	}

signals:
	void wifiEnabledChanged();
	void wifiHardwareEnabledChanged();
	void canCheckConnectivityChanged();
	void connectivityCheckEnabledChanged();
	void connectivityChanged();
};

} // namespace qs::network
