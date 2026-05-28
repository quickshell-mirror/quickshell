#pragma once

#include <qobject.h>

#include "../enums.hpp"
#include "../network.hpp"
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "active_connection.hpp"
#include "enums.hpp"
#include "settings.hpp"

namespace qs::network {

// NMNetwork aggregates NMActiveConnections and NMSettings of the same network.
class NMNetwork: public QObject {
	Q_OBJECT;

public:
	explicit NMNetwork(QObject* parent = nullptr);

	void addSettings(NMSettings* settings);
	void addActiveConnection(NMActiveConnection* active);
	void forget();
	void connect(const QString& devPath);
	void connectWithSettings(const QString& devPath, NMSettings* settings);

	// clang-format off
	[[nodiscard]] NMConnectionState::Enum state() const { return this->bState; }
	[[nodiscard]] bool known() const { return this->bKnown; }
	[[nodiscard]] NMConnectionStateReason::Enum reason() const { return this->bReason; }
	QBindable<NMDeviceStateReason::Enum> bindableDeviceFailReason() { return &this->bDeviceFailReason; }
	[[nodiscard]] NMDeviceStateReason::Enum deviceFailReason() const { return this->bDeviceFailReason; }
	[[nodiscard]] QList<NMSettings*> settings() const { return this->mSettings.values(); }
	[[nodiscard]] NMSettings* referenceSettings() const { return this->bReferenceSettings; }
	[[nodiscard]] virtual Network* frontend() = 0;
	QBindable<bool> bindableVisible() { return &this->bVisible; }
	[[nodiscard]] bool visible() const { return this->bVisible; }
	// clang-format on

signals:
	void requestDisconnect();
	void requestActivateConnection(const QString& settingsPath);
	void
	requestAddAndActivateConnection(const NMSettingsMap& settingsMap, const QString& specificObject);

	void settingsAdded(NMSettings* settings);
	void settingsRemoved(NMSettings* settings);
	void stateChanged(NMConnectionState::Enum state);
	void knownChanged(bool known);
	void reasonChanged(NMConnectionStateReason::Enum reason);
	void deviceFailReasonChanged(NMDeviceStateReason::Enum reason);
	void referenceSettingsChanged(NMSettings* settings);
	void visibilityChanged(bool visible);

protected:
	void bindFrontend(Network* frontend);
	QHash<QString, NMSettings*> mSettings;
	Q_OBJECT_BINDABLE_PROPERTY(
	    NMNetwork,
	    NMSettings*,
	    bReferenceSettings,
	    &NMNetwork::referenceSettingsChanged
	);

private:
	void updateReferenceSettings();

	NMActiveConnection* mActiveConnection = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMNetwork, bool, bVisible, &NMNetwork::visibilityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMNetwork, bool, bKnown, &NMNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMNetwork, NMConnectionStateReason::Enum, bReason, &NMNetwork::reasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMNetwork, NMConnectionState::Enum, bState, &NMNetwork::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMNetwork, NMDeviceStateReason::Enum, bDeviceFailReason, &NMNetwork::deviceFailReasonChanged);
	// clang-format on
};

// NMGenericNetwork extends NMNetwork to bind and handle the lifetime of a frontend Network.
// This is useful for devices with one network that don't need to extend the base Network class.
class NMGenericNetwork: public NMNetwork {
	Q_OBJECT;

public:
	explicit NMGenericNetwork(QString name, NetworkDevice* device, QObject* parent = nullptr);
	[[nodiscard]] Network* frontend() override { return this->mFrontend; }

private:
	void bindFrontend();
	Network* mFrontend;
};

// NMWirelessNetwork extends NMNetwork to also aggregate NMAccessPoints of the same network and scanning functionality.
class NMWirelessNetwork: public NMNetwork {
	Q_OBJECT;

public:
	explicit NMWirelessNetwork(const QString& ssid, NetworkDevice* device, QObject* parent = nullptr);

	void addAccessPoint(NMAccessPoint* ap);

	// clang-format off
	[[nodiscard]] QString ssid() const { return this->mSsid; }
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; }
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; }
	[[nodiscard]] NMAccessPoint* referenceAp() const { return this->bReferenceAp; }
	[[nodiscard]] QList<NMAccessPoint*> accessPoints() const { return this->mAccessPoints.values(); }
	QBindable<QString> bindableActiveApPath() { return &this->bActiveApPath; }
	[[nodiscard]] WifiNetwork* frontend() override { return this->mFrontend; };
	// clang-format on

signals:
	void disappeared();
	void signalStrengthChanged(quint8 signal);
	void securityChanged(WifiSecurityType::Enum security);
	void activeApPathChanged(QString path);
	void referenceApChanged(NMAccessPoint* ap);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void apRemoved(NMAccessPoint* ap);

private:
	void updateReferenceAp();
	void bindFrontend();

	WifiNetwork* mFrontend;
	QString mSsid;
	QHash<QString, NMAccessPoint*> mAccessPoints;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, WifiSecurityType::Enum, bSecurity, &NMWirelessNetwork::securityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, quint8, bSignalStrength, &NMWirelessNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, QString, bActiveApPath, &NMWirelessNetwork::activeApPathChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMAccessPoint*, bReferenceAp, &NMWirelessNetwork::referenceApChanged);
	// clang-format on
};

} // namespace qs::network
