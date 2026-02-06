#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "enums.hpp"

namespace qs::network {

///! A NetworkManager connection settings profile.
///
/// Secrets may be included in attempts to write to any of the settings properties below,
/// and will either be stored in persistent storage or sent to a Secret agent for storage,
/// depending on the flags associated with each secret and the presence of a registered Secret Agent.
/// > [!WARNING] Secrets aren't guranteed to be in sync here if they are stored in an agent.
class NMConnection: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// General connection profile settings.
	/// See https://networkmanager.dev/docs/api/latest/settings-connection.html.
	Q_PROPERTY(QVariantMap generalSettings READ default WRITE setGeneralSettings NOTIFY generalSettingsChanged BINDABLE bindableGeneralSettings);
	/// A human readable unique identifier for the connection.
	Q_PROPERTY(QString id READ default NOTIFY idChanged BINDABLE bindableId);
	// clang-format on

public:
	explicit NMConnection(QObject* parent = nullptr);
	/// Attempt to clear all of the secrets belonging to this connection.
	Q_INVOKABLE void clearSecrets();
	/// Delete the connection.
	Q_INVOKABLE void forget();

	// clang-format off
	QBindable<QVariantMap> bindableGeneralSettings() { return &this->bGeneralSettings; }
	void setGeneralSettings(const QVariantMap& settings);
	QBindable<QString> bindableId() { return &this->bId; }
	[[nodiscard]] QString id() const { return this->bId; }
	// clang-format on

signals:
	void requestClearSecrets();
	void requestForget();
	void requestSetGeneralSettings(QVariantMap settings);
	void generalSettingsChanged();
	void idChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnection, QVariantMap, bGeneralSettings, &NMConnection::generalSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnection, QString, bId, &NMConnection::idChanged);
	// clang-format on
};

/// Wi-Fi variant of a @@Quickshell.Networking.NMConnection.
class NMWifiConnection: public NMConnection {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// 802.11 Wi-Fi settings.
	/// See https://networkmanager.dev/docs/api/latest/settings-802-11-wireless.html.
	Q_PROPERTY(QVariantMap wifiSettings READ default WRITE setWifiSettings NOTIFY wifiSettingsChanged BINDABLE bindableWifiSettings);
	/// 802.11 Wi-Fi security settings.
	/// See https://networkmanager.dev/docs/api/latest/settings-802-11-wireless-security.html.
	Q_PROPERTY(QVariantMap wifiSecuritySettings READ default WRITE setWifiSecuritySettings NOTIFY wifiSecuritySettingsChanged BINDABLE bindableWifiSecuritySettings);
	/// 802.1x authentication settings.
	/// See https://networkmanager.dev/docs/api/latest/settings-802-1x.html.
	Q_PROPERTY(QVariantMap wifiAuthSettings READ default WRITE setWifiAuthSettings NOTIFY wifiAuthSettingsChanged BINDABLE bindableWifiAuthSettings);
	/// The wifi security type of the connection.
	Q_PROPERTY(WifiSecurityType::Enum wifiSecurity READ default NOTIFY wifiSecurityChanged BINDABLE bindableWifiSecurity);
	// clang-format on

public:
	explicit NMWifiConnection(QObject* parent = nullptr);
	/// Set the Pre-Shared-Key secret setting for a connection whos @@wifiSecurity is either
	/// @@Quickshell.Networking.WifiSecurityType.WpaPsk or @@Quickshell.Networking.WifiSecurityType.Wpa2Psk.
	Q_INVOKABLE void setWifiPsk(const QString& psk);

	// clang-format off
	QBindable<QVariantMap> bindableWifiSettings() { return &this->bWifiSettings; }
	void setWifiSettings(const QVariantMap& settings);
	QBindable<QVariantMap> bindableWifiSecuritySettings() { return &this->bWifiSecuritySettings; }
	void setWifiSecuritySettings(const QVariantMap& settings);
	QBindable<QVariantMap> bindableWifiAuthSettings() { return &this->bWifiAuthSettings; }
	void setWifiAuthSettings(const QVariantMap& settings);
	QBindable<WifiSecurityType::Enum> bindableWifiSecurity() { return &this->bWifiSecurity; }
	// clang-format on

signals:
	void requestSetWifiSettings(QVariantMap settings);
	void requestSetWifiSecuritySettings(QVariantMap settings);
	void requestSetWifiAuthSettings(QVariantMap settings);
	void requestSetWifiPsk(const QString& psk);
	void wifiSettingsChanged();
	void wifiSecuritySettingsChanged();
	void wifiAuthSettingsChanged();
	void wifiSecurityChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWifiConnection, QVariantMap, bWifiSettings, &NMWifiConnection::wifiSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWifiConnection, QVariantMap, bWifiSecuritySettings, &NMWifiConnection::wifiSecuritySettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWifiConnection, QVariantMap, bWifiAuthSettings, &NMWifiConnection::wifiAuthSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWifiConnection, WifiSecurityType::Enum, bWifiSecurity, &NMWifiConnection::wifiSecurityChanged);
	// clang-format on
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NMConnection* connection);
