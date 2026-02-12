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
	Q_PROPERTY(QString id READ default WRITE setId NOTIFY idChanged);
	Q_PROPERTY(bool autoconnect READ default WRITE setAutoconnect NOTIFY autoconnectChanged);
	Q_PROPERTY(qint32 autoconnectPriority READ default WRITE setAutoconnectPriority NOTIFY autoconnectPriorityChanged);
	Q_PROPERTY(qint32 dnsOverTls READ default WRITE setDnsOverTls NOTIFY dnsOverTlsChanged);
	Q_PROPERTY(qint32 dnssec READ default WRITE setDnssec NOTIFY dnssecChanged);
	// clang-format on

public:
	explicit NMSettings(QObject* parent = nullptr);
	/// Clear all of the secrets belonging to the settings.
	Q_INVOKABLE void clearSecrets();
	/// Delete the settings.
	Q_INVOKABLE void forget();

	// clang-format off
	QBindable<QString> bindableId() { return &this->bId; };
	void setId(const QString& id);
	[[nodiscard]] QString id() const { return this->bId; };
	QBindable<bool> bindableAutoconnect() { return &this->bAutoconnect; };
	void setAutoconnect(bool autoconnect);
	[[nodiscard]] bool autoconnect() const { return this->bAutoconnect; };
	QBindable<qint32> bindableAutoconnectPriority() { return &this->bAutoconnectPriority; };
	void setAutoconnectPriority(qint32 autoconnectPriority);
	[[nodiscard]] qint32 autoconnectPriority() const { return this->bAutoconnectPriority; };
	QBindable<qint32> bindableDnsOverTls() { return &this->bDnsOverTls; };
	void setDnsOverTls(qint32 dnsOverTls);
	[[nodiscard]] qint32 dnsOverTls() const { return this->bDnsOverTls; };
	QBindable<qint32> bindableDnssec() { return &this->bDnssec; };
	void setDnssec(qint32 dnssec);
	[[nodiscard]] qint32 dnssec() const { return this->bDnssec; };
	// clang-format on

signals:
	void requestClearSecrets();
	void requestForget();
	void requestSetId();
	void requestSetAutoconnect();
	void requestSetAutoconnectPriority();
	void requestSetDnsOverTls();
	void requestSetDnssec();
	void idChanged();
	void autoconnectChanged();
	void autoconnectPriorityChanged();
	void dnsOverTlsChanged();
	void dnssecChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, QString, bId, &NMSettings::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, bool, bAutoconnect, &NMSettings::autoconnectChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, qint32, bAutoconnectPriority, &NMSettings::autoconnectPriorityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, qint32, bDnsOverTls, &NMSettings::dnsOverTlsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, qint32, bDnssec, &NMSettings::dnssecChanged);
	// clang-format on
};

/// Wi-Fi variant of @@NMSettings.
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
	/// Only available for settings whos @@wifiSecurity is one of @@WifiSecurityType.WpaPsk, @@WifiSecurityType.Wpa2Psk, or @@WifiSecurityType.Sae.
	Q_INVOKABLE void setPsk(const QString& psk);

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
