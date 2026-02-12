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
class NMSettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// A human readable unique identifier for the settings.
	Q_PROPERTY(QString id READ default NOTIFY idChanged BINDABLE bindableId);
	// clang-format on

public:
	explicit NMSettings(QObject* parent = nullptr);
	/// Clear all of the secrets belonging to the settings.
	Q_INVOKABLE void clearSecrets();
	/// Delete the settings.
	Q_INVOKABLE void forget();

	// clang-format off
	QBindable<QString> bindableId() { return &this->bId; }
	[[nodiscard]] QString id() const { return this->bId; }
	// clang-format on

signals:
	void requestClearSecrets();
	void requestForget();
	void idChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, QString, bId, &NMSettings::idChanged);
	// clang-format on
};

/// Wi-Fi variant of a @@Quickshell.Networking.NMConnection.
class NMWifiSettings: public NMSettings {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// The wifi security type of the settings.
	Q_PROPERTY(WifiSecurityType::Enum wifiSecurity READ default NOTIFY wifiSecurityChanged BINDABLE bindableWifiSecurity);
	// clang-format on

public:
	explicit NMWifiSettings(QObject* parent = nullptr);
	/// Set the Pre-Shared-Key secret.
	/// Only available for settings whos @@wifiSecurity is one of @@Quickshell.Networking.WifiSecurityType.WpaPsk, @@Quickshell.Networking.WifiSecurityType.Wpa2Psk, or @@Quickshell.Networking.WifiSecurityType.Sae.
	Q_INVOKABLE void setWifiPsk(const QString& psk);

	QBindable<WifiSecurityType::Enum> bindableWifiSecurity() { return &this->bWifiSecurity; }

signals:
	void requestSetWifiPsk(const QString& psk);
	void wifiSecurityChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWifiSettings, WifiSecurityType::Enum, bWifiSecurity, &NMWifiSettings::wifiSecurityChanged);
	// clang-format on
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NMSettings* settings);
