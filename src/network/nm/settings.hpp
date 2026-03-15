#pragma once

#include <qbytearray.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qstringlist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "dbus_nm_connection_settings.h"
#include "types.hpp"

namespace qs::network {

///! Connection settings for a NetworkManager connection profile.
/// Details about the properties of this class can be found at
/// https://networkmanager.dev/docs/api/latest/settings-connection.html.
class NMConnectionSettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	/// A human readable unique identifier for the connection.
	Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged);
	/// A universal unique identifier for the connection.
	Q_PROPERTY(QString uuid READ uuid BINDABLE bindableUuid);
	/// An identifier used to deterministically generate stable values like IPv6 addresses, MAC addresses, and DHCP identifiers.
	Q_PROPERTY(QString stableId READ stableId WRITE setStableId NOTIFY stableIdChanged);
	/// Whether or not the connection should be automatically connected to.
	Q_PROPERTY(bool autoconnect READ autoconnect WRITE setAutoconnect NOTIFY autoconnectChanged);
	/// The autoconnect priority in range -999 to 999. Defaults to 0.
	Q_PROPERTY(qint32 autoconnectPriority READ autoconnectPriority WRITE setAutoconnectPriority NOTIFY autoconnectPriorityChanged);
	/// The number of times a connection should be tred when autoactivating before giving up.
	Q_PROPERTY(qint32 autoconnectRetries READ autoconnectRetries WRITE setAutoconnectRetries NOTIFY autoconnectRetriesChanged);
	/// Whether or not ports of this connection should be automatically brought up when this connection is activated.
	Q_PROPERTY(qint32 autoconnectPorts READ autoconnectPorts WRITE setAutoconnectPorts NOTIFY autoconnectPortsChanged);
	/// The number of retries for the authentication.
	Q_PROPERTY(qint32 authRetries READ authRetries WRITE setAuthRetries NOTIFY authRetriesChanged);
	/// Time time, in seconds since the Unix Epoch, that the connection was last fully activated.
	Q_PROPERTY(quint64 timestamp READ timestamp BINDABLE bindableTimestamp);
	/// An array of strings defining what access a given user has to this connection.
	Q_PROPERTY(QStringList permissions READ permissions WRITE setPermissions NOTIFY permissionsChanged);
	/// The firewall trust level of the connection.
	Q_PROPERTY(QString zone READ zone WRITE setZone NOTIFY zoneChanged);
	/// Setting name of the device type of this port's controller connection.
	Q_PROPERTY(QString portType READ portType WRITE setPortType NOTIFY portTypeChanged);
	/// Interface name of the controller device or UUID of the controller connection.
	Q_PROPERTY(QString controller READ controller WRITE setController NOTIFY controllerChanged);
	/// Whether DNSOverTls is enabled for the connection.
	Q_PROPERTY(qint32 dnsOverTls READ dnsOverTls WRITE setDnsOverTls NOTIFY dnsOverTlsChanged);
	/// Whether DNSSEC is enabled for the connection.
	Q_PROPERTY(qint32 dnssec READ dnssec WRITE setDnssec NOTIFY dnssecChanged);
	/// A list of target IP addresses for pinging.
	Q_PROPERTY(QStringList ipPingAddresses READ ipPingAddresses WRITE setIpPingAddresses NOTIFY ipPingAddressesChanged);
	/// Whether it is sufficient for any ping check to succeed among "ip-ping-addresses", or if all ping checks must succeed.
	Q_PROPERTY(qint32 ipPingAddressesRequireAll READ ipPingAddressesRequireAll WRITE setIpPingAddressesRequireAll NOTIFY ipPingAddressesRequireAllChanged);
	/// Delays the success of IP addressing until the specified timeout (seconds) is reached, or a target IP address replies to a ping.
	Q_PROPERTY(quint32 ipPingTimeout READ ipPingTimeout WRITE setIpPingTimeout NOTIFY ipPingTimeoutChanged);
	/// Whether LLDP is enabled for the connection.
	Q_PROPERTY(qint32 lldp READ lldp WRITE setLldp NOTIFY lldpChanged);
	/// Whether LLMNR is enabled for the connection.
	Q_PROPERTY(qint32 llmnr READ llmnr WRITE setLlmnr NOTIFY llmnrChanged);
	/// Whether mDNS is enabled for the connection.
	Q_PROPERTY(qint32 mdns READ mdns WRITE setMdns NOTIFY mdnsChanged);
	/// Whether the connection is metered.
	Q_PROPERTY(qint32 metered READ metered WRITE setMetered NOTIFY meteredChanged);
	/// A manufacturer usage description URL.
	Q_PROPERTY(QString mudUrl READ mudUrl WRITE setMudUrl NOTIFY mudUrlChanged);
	/// Specifies whether the profile can be active multiple times at a particular moment.
	Q_PROPERTY(qint32 multiConnect READ multiConnect WRITE setMultiConnect NOTIFY multiConnectChanged);
	/// List of connection UUIDs that should be activated when the base connection itself is activated.
	Q_PROPERTY(QStringList secondaries READ secondaries WRITE setSecondaries NOTIFY secondariesChanged);
	/// Time in milliseconds to wait for the device at startup.
	Q_PROPERTY(qint32 waitDeviceTimeout READ waitDeviceTimeout WRITE setWaitDeviceTimeout NOTIFY waitDeviceTimeoutChanged);
	/// Time in milliseconds to wait for the connection to be considered activated.
	Q_PROPERTY(qint32 waitActivationDelay READ waitActivationDelay WRITE setWaitActivationDelay NOTIFY waitActivationDelayChanged);
	// clang-format on

public:
	explicit NMConnectionSettings(QObject* parent = nullptr);

	// clang-format off
	[[nodiscard]] QString id() const { return this->bId; };
	void setId(const QString& id);
	QBindable<QString> bindableId() { return &this->bId; };
	[[nodiscard]] bool autoconnect() const { return this->bAutoconnect; };
	void setAutoconnect(bool autoconnect);
	QBindable<bool> bindableAutoconnect() { return &this->bAutoconnect; };
	[[nodiscard]] qint32 autoconnectPriority() const { return this->bAutoconnectPriority; };
	void setAutoconnectPriority(qint32 autoconnectPriority);
	QBindable<qint32> bindableAutoconnectPriority() { return &this->bAutoconnectPriority; };
	[[nodiscard]] qint32 autoconnectPorts() const { return this->bAutoconnectPorts; };
	void setAutoconnectPorts(qint32 autoconnectPorts);
	QBindable<qint32> bindableAutoconnectPorts() { return &this->bAutoconnectPorts; };
	[[nodiscard]] qint32 autoconnectRetries() const { return this->bAutoconnectRetries; };
	void setAutoconnectRetries(qint32 autoconnectRetries);
	QBindable<qint32> bindableAutoconnectRetries() { return &this->bAutoconnectRetries; };
	[[nodiscard]] qint32 authRetries() const { return this->bAuthRetries; };
	void setAuthRetries(qint32 authRetries);
	QBindable<qint32> bindableAuthRetries() { return &this->bAuthRetries; };
	[[nodiscard]] QString controller() const { return this->bController; };
	void setController(const QString& controller);
	QBindable<QString> bindableController() { return &this->bController; };
	[[nodiscard]] qint32 dnsOverTls() const { return this->bDnsOverTls; };
	void setDnsOverTls(qint32 dnsOverTls);
	QBindable<qint32> bindableDnsOverTls() { return &this->bDnsOverTls; };
	[[nodiscard]] qint32 dnssec() const { return this->bDnssec; };
	void setDnssec(qint32 dnssec);
	QBindable<qint32> bindableDnssec() { return &this->bDnssec; };
	[[nodiscard]] QStringList ipPingAddresses() const { return this->bIpPingAddresses; };
	void setIpPingAddresses(const QStringList& ipPingAddresses);
	QBindable<QStringList> bindableIpPingAddresses() { return &this->bIpPingAddresses; };
	[[nodiscard]] qint32 ipPingAddressesRequireAll() const { return this->bIpPingAddressesRequireAll; };
	void setIpPingAddressesRequireAll(qint32 ipPingAddressesRequireAll);
	QBindable<qint32> bindableIpPingAddressesRequireAll() { return &this->bIpPingAddressesRequireAll; };
	[[nodiscard]] quint32 ipPingTimeout() const { return this->bIpPingTimeout; };
	void setIpPingTimeout(quint32 ipPingTimeout);
	QBindable<quint32> bindableIpPingTimeout() { return &this->bIpPingTimeout; };
	[[nodiscard]] qint32 lldp() const { return this->bLldp; };
	void setLldp(qint32 lldp);
	QBindable<qint32> bindableLldp() { return &this->bLldp; };
	[[nodiscard]] qint32 llmnr() const { return this->bLlmnr; };
	void setLlmnr(qint32 llmnr);
	QBindable<qint32> bindableLlmnr() { return &this->bLlmnr; };
	[[nodiscard]] qint32 mdns() const { return this->bMdns; };
	void setMdns(qint32 mdns);
	QBindable<qint32> bindableMdns() { return &this->bMdns; };
	[[nodiscard]] qint32 metered() const { return this->bMetered; };
	void setMetered(qint32 metered);
	QBindable<qint32> bindableMetered() { return &this->bMetered; };
	[[nodiscard]] QString mudUrl() const { return this->bMudUrl; };
	void setMudUrl(const QString& mudUrl);
	QBindable<QString> bindableMudUrl() { return &this->bMudUrl; };
	[[nodiscard]] qint32 multiConnect() const { return this->bMultiConnect; };
	void setMultiConnect(qint32 multiConnect);
	QBindable<qint32> bindableMultiConnect() { return &this->bMultiConnect; };
	[[nodiscard]] QStringList permissions() const { return this->bPermissions; };
	void setPermissions(const QStringList& permissions);
	QBindable<QStringList> bindablePermissions() { return &this->bPermissions; };
	[[nodiscard]] QString portType() const { return this->bPortType; };
	void setPortType(const QString& portType);
	QBindable<QString> bindablePortType() { return &this->bPortType; };
	[[nodiscard]] QStringList secondaries() const { return this->bSecondaries; };
	void setSecondaries(const QStringList& secondaries);
	QBindable<QStringList> bindableSecondaries() { return &this->bSecondaries; };
	[[nodiscard]] QString stableId() const { return this->bStableId; };
	void setStableId(const QString& stableId);
	QBindable<QString> bindableStableId() { return &this->bStableId; };
	[[nodiscard]] quint64 timestamp() const { return this->bTimestamp; };
	QBindable<quint64> bindableTimestamp() { return &this->bTimestamp; };
	[[nodiscard]] QString uuid() const { return this->bUuid; };
	QBindable<QString> bindableUuid() { return &this->bUuid; };
	[[nodiscard]] qint32 waitActivationDelay() const { return this->bWaitActivationDelay; };
	void setWaitActivationDelay(qint32 waitActivationDelay);
	QBindable<qint32> bindableWaitActivationDelay() { return &this->bWaitActivationDelay; };
	[[nodiscard]] qint32 waitDeviceTimeout() const { return this->bWaitDeviceTimeout; };
	void setWaitDeviceTimeout(qint32 waitDeviceTimeout);
	QBindable<qint32> bindableWaitDeviceTimeout() { return &this->bWaitDeviceTimeout; };
	[[nodiscard]] QString zone() const { return this->bZone; };
	void setZone(const QString& zone);
	QBindable<QString> bindableZone() { return &this->bZone; };
	// clang-format on

	QBindable<QVariantMap> bindableSettings() { return &this->bSettings; };

signals:
	void settingsChanged();
	void idChanged();
	void autoconnectChanged();
	void autoconnectPriorityChanged();
	void autoconnectPortsChanged();
	void autoconnectRetriesChanged();
	void authRetriesChanged();
	void controllerChanged();
	void dnsOverTlsChanged();
	void dnssecChanged();
	void ipPingAddressesChanged();
	void ipPingAddressesRequireAllChanged();
	void ipPingTimeoutChanged();
	void lldpChanged();
	void llmnrChanged();
	void mdnsChanged();
	void meteredChanged();
	void mudUrlChanged();
	void multiConnectChanged();
	void permissionsChanged();
	void portTypeChanged();
	void secondariesChanged();
	void stableIdChanged();
	void timestampChanged();
	void uuidChanged();
	void waitActivationDelayChanged();
	void waitDeviceTimeoutChanged();
	void zoneChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QVariantMap, bSettings, &NMConnectionSettings::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bId, &NMConnectionSettings::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, bool, bAutoconnect, &NMConnectionSettings::autoconnectChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bAutoconnectPriority, &NMConnectionSettings::autoconnectPriorityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bAutoconnectPorts, &NMConnectionSettings::autoconnectPortsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bAutoconnectRetries, &NMConnectionSettings::autoconnectRetriesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bAuthRetries, &NMConnectionSettings::authRetriesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bController, &NMConnectionSettings::controllerChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bDnsOverTls, &NMConnectionSettings::dnsOverTlsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bDnssec, &NMConnectionSettings::dnssecChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QStringList, bIpPingAddresses, &NMConnectionSettings::ipPingAddressesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bIpPingAddressesRequireAll, &NMConnectionSettings::ipPingAddressesRequireAllChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, quint32, bIpPingTimeout, &NMConnectionSettings::ipPingTimeoutChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bLldp, &NMConnectionSettings::lldpChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bLlmnr, &NMConnectionSettings::llmnrChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bMdns, &NMConnectionSettings::mdnsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bMetered, &NMConnectionSettings::meteredChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bMudUrl, &NMConnectionSettings::mudUrlChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bMultiConnect, &NMConnectionSettings::multiConnectChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QStringList, bPermissions, &NMConnectionSettings::permissionsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bPortType, &NMConnectionSettings::portTypeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QStringList, bSecondaries, &NMConnectionSettings::secondariesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bStableId, &NMConnectionSettings::stableIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, quint64, bTimestamp, &NMConnectionSettings::timestampChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bUuid, &NMConnectionSettings::uuidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bWaitActivationDelay, &NMConnectionSettings::waitActivationDelayChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, qint32, bWaitDeviceTimeout, &NMConnectionSettings::waitDeviceTimeoutChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, QString, bZone, &NMConnectionSettings::zoneChanged);
	// clang-format on
};

///! Wifi settings for a NetworkManager connection profile.
/// Details about the properties of this class can be found at
/// https://networkmanager.dev/docs/api/latest/settings-802-11-wireless.html.
class NMWirelessSettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	/// SSID of the Wi-Fi network.
	Q_PROPERTY(QByteArray ssid READ ssid BINDABLE bindableSsid);
	/// A list of BSSIDs (formatted as a MAC address) that have been detected as part of the Wi-Fi network.
	Q_PROPERTY(QStringList seenBssids READ seenBssids BINDABLE bindableSeenBssids);
	/// One of 2 (disable), 3 (enable), 1 (ignore), or 0 (default; use the globally configured value).
	Q_PROPERTY(quint32 powersave READ powersave WRITE setPowersave NOTIFY powersaveChanged);
	/// If non-zero, only transmit packets of the specified size or smaller, breaking larger packets up into multiple Ethernet frames.
	Q_PROPERTY(quint32 mtu READ mtu WRITE setMtu NOTIFY mtuChanged);
	/// Wi-Fi network mode; one of "infrastructure", "mesh", "adhoc", or "ap". If blank, infrastructure is assumed.
	Q_PROPERTY(QString mode READ mode BINDABLE bindableMode);
	/// Configures AP isolation, which prevents communication between wireless devices connected to this AP.
	Q_PROPERTY(qint32 apIsolation READ apIsolation WRITE setApIsolation NOTIFY apIsolationChanged);
	/// 802.11 frequency band of the network. One of "a" for 5GHz 802.11a or "bg" for 2.4GHz 802.11.
	Q_PROPERTY(QString band READ band WRITE setBand NOTIFY bandChanged);
	/// If specified, directs the device to only associate with the given access point.
	Q_PROPERTY(QByteArray bssid READ bssid WRITE setBssid NOTIFY bssidChanged);
	/// Wireless channel to use for the Wi-Fi connection.
	Q_PROPERTY(quint32 channel READ channel WRITE setChannel NOTIFY channelChanged);
	/// Specifies width of the wireless channel in AP mode.
	Q_PROPERTY(qint32 channelWidth READ channelWidth WRITE setChannelWidth NOTIFY channelWidthChanged);
	/// If true, indicates that the network is a non-broadcasting network that hides its SSID.
	Q_PROPERTY(bool hidden READ hidden WRITE setHidden NOTIFY hiddenChanged);
	/// The cloned MAC address. It can either be a hardware address in ASCII representation, or one of the special values "preserve", "permanent", "random", or "stable".
	Q_PROPERTY(QString assignedMacAddress READ assignedMacAddress WRITE setAssignedMacAddress NOTIFY assignedMacAddressChanged);
	/// Specify that certain bits are fixed when the assigned-mac-address is set to "random" or "stable".
	Q_PROPERTY(QString generateMacAddressMask READ generateMacAddressMask WRITE setGenerateMacAddressMask NOTIFY generateMacAddressMaskChanged);
	/// If specified, this connection will only apply to the Wi-Fi device whos permanent MAC address matches.
	Q_PROPERTY(QByteArray macAddress READ macAddress WRITE setMacAddress NOTIFY macAddressChanged);
	/// A list of permanent MAC addresses of Wi-Fi devices to which this connection should never apply.
	Q_PROPERTY(QStringList macAddressBlacklist READ macAddressBlacklist WRITE setMacAddressBlacklist NOTIFY macAddressBlacklistChanged);
	/// One of 0 (global default), 1 (never randomize the MAC address), and 2 (always randomize the MAC address).
	Q_PROPERTY(quint32 macAddressRandomization READ macAddressRandomization WRITE setMacAddressRandomization NOTIFY macAddressRandomizationChanged);
	// clang-format on

public:
	explicit NMWirelessSettings(QObject* parent = nullptr);

	// clang-format off
	[[nodiscard]] QByteArray ssid() const { return this->bSsid; };
	QBindable<QByteArray> bindableSsid() { return &this->bSsid; };
	[[nodiscard]] QStringList seenBssids() const { return this->bSeenBssids; };
	QBindable<QStringList> bindableSeenBssids() { return &this->bSeenBssids; };
	[[nodiscard]] quint32 powersave() const { return this->bPowersave; };
	void setPowersave(quint32 powersave);
	QBindable<quint32> bindablePowersave() { return &this->bPowersave; };
	[[nodiscard]] quint32 mtu() const { return this->bMtu; };
	void setMtu(quint32 mtu);
	QBindable<quint32> bindableMtu() { return &this->bMtu; };
	[[nodiscard]] QString mode() const { return this->bMode; };
	QBindable<QString> bindableMode() { return &this->bMode; };
	[[nodiscard]] qint32 apIsolation() const { return this->bApIsolation; };
	void setApIsolation(qint32 apIsolation);
	QBindable<qint32> bindableApIsolation() { return &this->bApIsolation; };
	[[nodiscard]] QString band() const { return this->bBand; };
	void setBand(const QString& band);
	QBindable<QString> bindableBand() { return &this->bBand; };
	[[nodiscard]] QByteArray bssid() const { return this->bBssid; };
	void setBssid(const QByteArray& bssid);
	QBindable<QByteArray> bindableBssid() { return &this->bBssid; };
	[[nodiscard]] quint32 channel() const { return this->bChannel; };
	void setChannel(quint32 channel);
	QBindable<quint32> bindableChannel() { return &this->bChannel; };
	[[nodiscard]] qint32 channelWidth() const { return this->bChannelWidth; };
	void setChannelWidth(qint32 channelWidth);
	QBindable<qint32> bindableChannelWidth() { return &this->bChannelWidth; };
	[[nodiscard]] bool hidden() const { return this->bHidden; };
	void setHidden(bool hidden);
	QBindable<bool> bindableHidden() { return &this->bHidden; };
	[[nodiscard]] QString assignedMacAddress() const { return this->bAssignedMacAddress; };
	void setAssignedMacAddress(const QString& assignedMacAddress);
	QBindable<QString> bindableAssignedMacAddress() { return &this->bAssignedMacAddress; };
	[[nodiscard]] QString generateMacAddressMask() const { return this->bGenerateMacAddressMask; };
	void setGenerateMacAddressMask(const QString& generateMacAddressMask);
	QBindable<QString> bindableGenerateMacAddressMask() { return &this->bGenerateMacAddressMask; };
	[[nodiscard]] QByteArray macAddress() const { return this->bMacAddress; };
	void setMacAddress(const QByteArray& macAddress);
	QBindable<QByteArray> bindableMacAddress() { return &this->bMacAddress; };
	[[nodiscard]] QStringList macAddressBlacklist() const { return this->bMacAddressBlacklist; };
	void setMacAddressBlacklist(const QStringList& macAddressBlacklist);
	QBindable<QStringList> bindableMacAddressBlacklist() { return &this->bMacAddressBlacklist; };
	[[nodiscard]] quint32 macAddressRandomization() const { return this->bMacAddressRandomization; };
	void setMacAddressRandomization(quint32 macAddressRandomization);
	QBindable<quint32> bindableMacAddressRandomization() { return &this->bMacAddressRandomization; };
	// clang-format on

	QBindable<QVariantMap> bindableSettings() { return &this->bSettings; };

signals:
	void settingsChanged();
	void ssidChanged();
	void seenBssidsChanged();
	void powersaveChanged();
	void mtuChanged();
	void modeChanged();
	void apIsolationChanged();
	void bandChanged();
	void bssidChanged();
	void channelChanged();
	void channelWidthChanged();
	void hiddenChanged();
	void assignedMacAddressChanged();
	void generateMacAddressMaskChanged();
	void macAddressChanged();
	void macAddressBlacklistChanged();
	void macAddressRandomizationChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QVariantMap, bSettings, &NMWirelessSettings::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QByteArray, bSsid, &NMWirelessSettings::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QStringList, bSeenBssids, &NMWirelessSettings::seenBssidsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, quint32, bPowersave, &NMWirelessSettings::powersaveChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, quint32, bMtu, &NMWirelessSettings::mtuChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QString, bMode, &NMWirelessSettings::modeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, qint32, bApIsolation, &NMWirelessSettings::apIsolationChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QString, bBand, &NMWirelessSettings::bandChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QByteArray, bBssid, &NMWirelessSettings::bssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, quint32, bChannel, &NMWirelessSettings::channelChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, qint32, bChannelWidth, &NMWirelessSettings::channelWidthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, bool, bHidden, &NMWirelessSettings::hiddenChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QString, bAssignedMacAddress, &NMWirelessSettings::assignedMacAddressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QString, bGenerateMacAddressMask, &NMWirelessSettings::generateMacAddressMaskChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QByteArray, bMacAddress, &NMWirelessSettings::macAddressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, QStringList, bMacAddressBlacklist, &NMWirelessSettings::macAddressBlacklistChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSettings, quint32, bMacAddressRandomization, &NMWirelessSettings::macAddressRandomizationChanged);
	// clang-format on
};

///! Wifi security settings for a NetworkManager connection profile.
/// Details about the properties of this class can be found at
/// https://networkmanager.dev/docs/api/latest/settings-802-11-wireless-security.html.
class NMWirelessSecuritySettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	/// When WEP is used indicate the 802.11 authentication algorithm required by the AP here.
	Q_PROPERTY(QString authAlg READ authAlg WRITE setAuthAlg NOTIFY authAlgChanged);
	/// Indicates whether Fast Initial Link Setup (802.11ai) must be enabled for the connection.
	Q_PROPERTY(qint32 fils READ fils WRITE setFils NOTIFY filsChanged);
	/// A lift of group/broadcast encryption algorithms which prevents connections to the Wi-Fi networks that do not utulize one of the algorithms in this list.
	Q_PROPERTY(QStringList group READ group WRITE setGroup NOTIFY groupChanged);
	/// Key management used for the connection.
	Q_PROPERTY(QString keyMgmt READ keyMgmt WRITE setKeyMgmt NOTIFY keyMgmtChanged);
	/// The login password for legacy LEAP connections.
	Q_PROPERTY(QString leapUsername READ leapUsername WRITE setLeapUsername NOTIFY leapUsernameChanged);
	/// A list of pairwise encryption algorithms which prevents connections to Wi-Fi networks that do not utilize one of the algorithms in the list.
	Q_PROPERTY(QStringList pairwise READ pairwise WRITE setPairwise NOTIFY pairwiseChanged);
	/// Indicates whether Protected Management Frames (802.11w) must be enabled for the connection.
	Q_PROPERTY(qint32 pmf READ pmf WRITE setPmf NOTIFY pmfChanged);
	/// List of strings specifying the allowed WPA protocol versions to use.
	Q_PROPERTY(QStringList proto READ proto WRITE setProto NOTIFY protoChanged);
	/// Controls the interpretation of WEP keys.
	Q_PROPERTY(quint32 wepKeyType READ wepKeyType WRITE setWepKeyType NOTIFY wepKeyTypeChanged);
	/// Flags indicating which mode of WPS is to be used if any.
	Q_PROPERTY(quint32 wpsMethod READ wpsMethod WRITE setWpsMethod NOTIFY wpsMethodChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the LEAP password.
	Q_PROPERTY(quint32 leapPasswordFlags READ leapPasswordFlags WRITE setLeapPasswordFlags NOTIFY leapPasswordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the PSK.
	Q_PROPERTY(quint32 pskFlags READ pskFlags WRITE setPskFlags NOTIFY pskFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the WEP key.
	Q_PROPERTY(quint32 wepKeyFlags READ wepKeyFlags WRITE setWepKeyFlags NOTIFY wepKeyFlagsChanged);
	// clang-format on

public:
	explicit NMWirelessSecuritySettings(QObject* parent = nullptr);

	// clang-format off
	[[nodiscard]] QString authAlg() const { return this->bAuthAlg; };
	void setAuthAlg(const QString& authAlg);
	QBindable<QString> bindableAuthAlg() { return &this->bAuthAlg; };
	[[nodiscard]] qint32 fils() const { return this->bFils; };
	void setFils(qint32 fils);
	QBindable<qint32> bindableFils() { return &this->bFils; };
	[[nodiscard]] QStringList group() const { return this->bGroup; };
	void setGroup(const QStringList& group);
	QBindable<QStringList> bindableGroup() { return &this->bGroup; };
	[[nodiscard]] QString keyMgmt() const { return this->bKeyMgmt; };
	void setKeyMgmt(const QString& keyMgmt);
	QBindable<QString> bindableKeyMgmt() { return &this->bKeyMgmt; };
	[[nodiscard]] QString leapUsername() const { return this->bLeapUsername; };
	void setLeapUsername(const QString& leapUsername);
	QBindable<QString> bindableLeapUsername() { return &this->bLeapUsername; };
	[[nodiscard]] QStringList pairwise() const { return this->bPairwise; };
	void setPairwise(const QStringList& pairwise);
	QBindable<QStringList> bindablePairwise() { return &this->bPairwise; };
	[[nodiscard]] qint32 pmf() const { return this->bPmf; };
	void setPmf(qint32 pmf);
	QBindable<qint32> bindablePmf() { return &this->bPmf; };
	[[nodiscard]] QStringList proto() const { return this->bProto; };
	void setProto(const QStringList& proto);
	QBindable<QStringList> bindableProto() { return &this->bProto; };
	[[nodiscard]] quint32 wepKeyType() const { return this->bWepKeyType; };
	void setWepKeyType(quint32 wepKeyType);
	QBindable<quint32> bindableWepKeyType() { return &this->bWepKeyType; };
	[[nodiscard]] quint32 wpsMethod() const { return this->bWpsMethod; };
	void setWpsMethod(quint32 wpsMethod);
	QBindable<quint32> bindableWpsMethod() { return &this->bWpsMethod; };
	[[nodiscard]] quint32 leapPasswordFlags() const { return this->bLeapPasswordFlags; };
	void setLeapPasswordFlags(quint32 leapPasswordFlags);
	QBindable<quint32> bindableLeapPasswordFlags() { return &this->bLeapPasswordFlags; };
	[[nodiscard]] quint32 pskFlags() const { return this->bPskFlags; };
	void setPskFlags(quint32 pskFlags);
	QBindable<quint32> bindablePskFlags() { return &this->bPskFlags; };
	[[nodiscard]] quint32 wepKeyFlags() const { return this->bWepKeyFlags; };
	void setWepKeyFlags(quint32 wepKeyFlags);
	QBindable<quint32> bindableWepKeyFlags() { return &this->bWepKeyFlags; };
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; };
	QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; };
	// clang-format on

	QBindable<QVariantMap> bindableSettings() { return &this->bSettings; };

	Q_INVOKABLE void setPsk(const QString& psk);
	Q_INVOKABLE void setWepKey(const QString& key, quint32 index, quint32 type);
	Q_INVOKABLE void setLeapPassword(const QString& password);

signals:
	void settingsChanged();
	void authAlgChanged();
	void filsChanged();
	void groupChanged();
	void keyMgmtChanged();
	void leapUsernameChanged();
	void pairwiseChanged();
	void pmfChanged();
	void protoChanged();
	void wepKeyTypeChanged();
	void wpsMethodChanged();
	void leapPasswordFlagsChanged();
	void pskFlagsChanged();
	void wepKeyFlagsChanged();
	void securityChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QVariantMap, bSettings, &NMWirelessSecuritySettings::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QString, bAuthAlg, &NMWirelessSecuritySettings::authAlgChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, qint32, bFils, &NMWirelessSecuritySettings::filsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QStringList, bGroup, &NMWirelessSecuritySettings::groupChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QString, bKeyMgmt, &NMWirelessSecuritySettings::keyMgmtChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QString, bLeapUsername, &NMWirelessSecuritySettings::leapUsernameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QStringList, bPairwise, &NMWirelessSecuritySettings::pairwiseChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, qint32, bPmf, &NMWirelessSecuritySettings::pmfChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, QStringList, bProto, &NMWirelessSecuritySettings::protoChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, quint32, bWepKeyType, &NMWirelessSecuritySettings::wepKeyTypeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, quint32, bWpsMethod, &NMWirelessSecuritySettings::wpsMethodChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, quint32, bLeapPasswordFlags, &NMWirelessSecuritySettings::leapPasswordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, quint32, bPskFlags, &NMWirelessSecuritySettings::pskFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, quint32, bWepKeyFlags, &NMWirelessSecuritySettings::wepKeyFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessSecuritySettings, WifiSecurityType::Enum, bSecurity, &NMWirelessSecuritySettings::securityChanged);
	// clang-format on
};

///! 802.1x authentication settings for a NetworkManager connection profile.
/// Details about the properties of this class can be found at
/// https://networkmanager.dev/docs/api/latest/settings-802-1x.html.
class NM8021xSettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	/// Array of strings to be matched against the altSubjectName of the certificate presented by the auth server.
	Q_PROPERTY(QStringList altsubjectMatches READ altsubjectMatches WRITE setAltsubjectMatches NOTIFY altsubjectMatchesChanged);
	/// Anonymous identity string for EAP authentication methods.
	Q_PROPERTY(QString anonymousIdentity READ anonymousIdentity WRITE setAnonymousIdentity NOTIFY anonymousIdentityChanged);
	/// A timeout for the authentication. Zero means the global default.
	Q_PROPERTY(qint32 authTimeout READ authTimeout WRITE setAuthTimeout NOTIFY authTimeoutChanged);
	/// Contains the CA certificate if used by the EAP method specified in @@eap.
	Q_PROPERTY(QByteArray caCert READ caCert WRITE setCaCert NOTIFY caCertChanged);
	/// UTF-8 encoded path to a directory containing PEM or DER formatted certificates to be added to the verification chain in addition to the certificate specified in the "ca-cert" property.
	Q_PROPERTY(QString caPath READ caPath WRITE setCaPath NOTIFY caPathChanged);
	/// Contains the client certificate if used by the EAP method specified in @@eap.
	Q_PROPERTY(QByteArray clientCert READ clientCert WRITE setClientCert NOTIFY clientCertChanged);
	/// Constraint for server domain name.
	Q_PROPERTY(QString domainMatch READ domainMatch WRITE setDomainMatch NOTIFY domainMatchChanged);
	/// The allowed EAP methods. Each element must be one of "leap", "md5", "tls", "peap", "ttls", "pwd", or "fast".
	Q_PROPERTY(QStringList eap READ eap WRITE setEap NOTIFY eapChanged);
	/// Identity string for EAP authentication methods.
	Q_PROPERTY(QString identity READ identity WRITE setIdentity NOTIFY identityChanged);
	/// Define openssl_ciphers for wpa_supplicant.
	Q_PROPERTY(QString opensslCiphers READ opensslCiphers WRITE setOpensslCiphers NOTIFY opensslCiphersChanged);
	/// Whether 802.1x authentication is optional.
	Q_PROPERTY(bool optional READ optional WRITE setOptional NOTIFY optionalChanged);
	/// UTF-8 encoded file path containing PAC for EAP-FAST.
	Q_PROPERTY(QString pacFile READ pacFile WRITE setPacFile NOTIFY pacFileChanged);
	/// Specifies authentication flags to use in "phase 1" outer authentication.
	Q_PROPERTY(quint32 phase1AuthFlags READ phase1AuthFlags WRITE setPhase1AuthFlags NOTIFY phase1AuthFlagsChanged);
	/// Enables or disables in-line provisioning of EAP-FAST credentials when FAST is specified as the EAP method in @@eap.
	Q_PROPERTY(QString phase1FastProvisioning READ phase1FastProvisioning WRITE setPhase1FastProvisioning NOTIFY phase1FastProvisioningChanged);
	/// Forces use of the new PEAP label during key derivation.
	Q_PROPERTY(QString phase1Peaplabel READ phase1Peaplabel WRITE setPhase1Peaplabel NOTIFY phase1PeaplabelChanged);
	/// List of strings to be matched against the altSubjectName of the certificate presented by the authentication server during the inner "phase 2" authentication.
	Q_PROPERTY(QStringList phase2AltsubjectMatches READ phase2AltsubjectMatches WRITE setPhase2AltsubjectMatches NOTIFY phase2AltsubjectMatchesChanged);
	/// Specifies the allowed "phase 2" inner auth method when an EAP method that uses inner TLS tunnel is specified in the @@eap property.
	Q_PROPERTY(QString phase2Auth READ phase2Auth WRITE setPhase2Auth NOTIFY phase2AuthChanged);
	/// Specifies the allowed "phase 2" inner EAP-based authentication method when TTLS is specified in the "eap" property.
	Q_PROPERTY(QString phase2Autheap READ phase2Autheap WRITE setPhase2Autheap NOTIFY phase2AutheapChanged);
	/// Contains the "phase 2" CA certificate if used by the EAP method specified in the "phase2-auth" or "phase2-autheap" properties.
	Q_PROPERTY(QByteArray phase2CaCert READ phase2CaCert WRITE setPhase2CaCert NOTIFY phase2CaCertChanged);
	/// UTF-8 encoded path to a directory containing PEM or DER formatted certificates to be added to the verification chain in addition to the certificate specified in the "phase2-ca-cert" property.
	Q_PROPERTY(QString phase2CaPath READ phase2CaPath WRITE setPhase2CaPath NOTIFY phase2CaPathChanged);
	/// Contains the "phase 2" client certificate if used by the EAP method specified in the "phase2-auth" or "phase2-autheap" properties.
	Q_PROPERTY(QByteArray phase2ClientCert READ phase2ClientCert WRITE setPhase2ClientCert NOTIFY phase2ClientCertChanged);
	/// Constraint for server domain name. If set, this list of FQDNs is used as a match requirement for dNSName element(s) of the certificate presented by the authentication server during the inner "phase 2" authentication.
	Q_PROPERTY(QString phase2DomainMatch READ phase2DomainMatch WRITE setPhase2DomainMatch NOTIFY phase2DomainMatchChanged);
	/// Constraint for server domain name. If set, this FQDN is used as a suffix match requirement for dNSName element(s) of the certificate presented by the authentication server during the inner "phase 2" authentication.
	Q_PROPERTY(QString phase2DomainSuffixMatch READ phase2DomainSuffixMatch WRITE setPhase2DomainSuffixMatch NOTIFY phase2DomainSuffixMatchChanged);
	/// Contains the "phase 2" inner private key when the "phase2-auth" or "phase2-autheap" property is set to "tls".
	Q_PROPERTY(QByteArray phase2PrivateKey READ phase2PrivateKey WRITE setPhase2PrivateKey NOTIFY phase2PrivateKeyChanged);
	/// Substring to be matched against the subject of the certificate presented by the authentication server during the inner "phase 2" authentication.
	Q_PROPERTY(QString phase2SubjectMatch READ phase2SubjectMatch WRITE setPhase2SubjectMatch NOTIFY phase2SubjectMatchChanged);
	/// Substring to be matched against the subject of the certificate presented by the authentication server.
	Q_PROPERTY(QString subjectMatch READ subjectMatch WRITE setSubjectMatch NOTIFY subjectMatchChanged);
	/// When TRUE, overrides the "ca-path" and "phase2-ca-path" properties using the system CA directory specified at configure time with the --system-ca-path switch.
	Q_PROPERTY(bool systemCaCerts READ systemCaCerts WRITE setSystemCaCerts NOTIFY systemCaCertsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the CA certificate password.
	Q_PROPERTY(quint32 caCertPasswordFlags READ caCertPasswordFlags WRITE setCaCertPasswordFlags NOTIFY caCertPasswordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the client certificate password.
	Q_PROPERTY(quint32 clientCertPasswordFlags READ clientCertPasswordFlags WRITE setClientCertPasswordFlags NOTIFY clientCertPasswordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the password.
	Q_PROPERTY(quint32 passwordFlags READ passwordFlags WRITE setPasswordFlags NOTIFY passwordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the raw password.
	Q_PROPERTY(quint32 passwordRawFlags READ passwordRawFlags WRITE setPasswordRawFlags NOTIFY passwordRawFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the "phase 2" CA certificate password.
	Q_PROPERTY(quint32 phase2CaCertPasswordFlags READ phase2CaCertPasswordFlags WRITE setPhase2CaCertPasswordFlags NOTIFY phase2CaCertPasswordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the "phase 2" client certificate password.
	Q_PROPERTY(quint32 phase2ClientCertPasswordFlags READ phase2ClientCertPasswordFlags WRITE setPhase2ClientCertPasswordFlags NOTIFY phase2ClientCertPasswordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the "phase 2" private key password.
	Q_PROPERTY(quint32 phase2PrivateKeyPasswordFlags READ phase2PrivateKeyPasswordFlags WRITE setPhase2PrivateKeyPasswordFlags NOTIFY phase2PrivateKeyPasswordFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the PIN.
	Q_PROPERTY(quint32 pinFlags READ pinFlags WRITE setPinFlags NOTIFY pinFlagsChanged);
	/// Bitfield of @@NMSecretFlag logically ORed indicating how to handle the private key password.
	Q_PROPERTY(quint32 privateKeyPasswordFlags READ privateKeyPasswordFlags WRITE setPrivateKeyPasswordFlags NOTIFY privateKeyPasswordFlagsChanged);
	// clang-format on

public:
	explicit NM8021xSettings(QObject* parent = nullptr);

	// clang-format off
	[[nodiscard]] QStringList altsubjectMatches() const { return this->bAltsubjectMatches; };
	void setAltsubjectMatches(const QStringList& altsubjectMatches);
	QBindable<QStringList> bindableAltsubjectMatches() { return &this->bAltsubjectMatches; };
	[[nodiscard]] QString anonymousIdentity() const { return this->bAnonymousIdentity; };
	void setAnonymousIdentity(const QString& anonymousIdentity);
	QBindable<QString> bindableAnonymousIdentity() { return &this->bAnonymousIdentity; };
	[[nodiscard]] qint32 authTimeout() const { return this->bAuthTimeout; };
	void setAuthTimeout(qint32 authTimeout);
	QBindable<qint32> bindableAuthTimeout() { return &this->bAuthTimeout; };
	[[nodiscard]] QByteArray caCert() const { return this->bCaCert; };
	void setCaCert(const QByteArray& caCert);
	QBindable<QByteArray> bindableCaCert() { return &this->bCaCert; };
	[[nodiscard]] QString caPath() const { return this->bCaPath; };
	void setCaPath(const QString& caPath);
	QBindable<QString> bindableCaPath() { return &this->bCaPath; };
	[[nodiscard]] QByteArray clientCert() const { return this->bClientCert; };
	void setClientCert(const QByteArray& clientCert);
	QBindable<QByteArray> bindableClientCert() { return &this->bClientCert; };
	[[nodiscard]] QString domainMatch() const { return this->bDomainMatch; };
	void setDomainMatch(const QString& domainMatch);
	QBindable<QString> bindableDomainMatch() { return &this->bDomainMatch; };
	[[nodiscard]] QStringList eap() const { return this->bEap; };
	void setEap(const QStringList& eap);
	QBindable<QStringList> bindableEap() { return &this->bEap; };
	[[nodiscard]] QString identity() const { return this->bIdentity; };
	void setIdentity(const QString& identity);
	QBindable<QString> bindableIdentity() { return &this->bIdentity; };
	[[nodiscard]] QString opensslCiphers() const { return this->bOpensslCiphers; };
	void setOpensslCiphers(const QString& opensslCiphers);
	QBindable<QString> bindableOpensslCiphers() { return &this->bOpensslCiphers; };
	[[nodiscard]] bool optional() const { return this->bOptional; };
	void setOptional(bool optional);
	QBindable<bool> bindableOptional() { return &this->bOptional; };
	[[nodiscard]] QString pacFile() const { return this->bPacFile; };
	void setPacFile(const QString& pacFile);
	QBindable<QString> bindablePacFile() { return &this->bPacFile; };
	[[nodiscard]] quint32 phase1AuthFlags() const { return this->bPhase1AuthFlags; };
	void setPhase1AuthFlags(quint32 phase1AuthFlags);
	QBindable<quint32> bindablePhase1AuthFlags() { return &this->bPhase1AuthFlags; };
	[[nodiscard]] QString phase1FastProvisioning() const { return this->bPhase1FastProvisioning; };
	void setPhase1FastProvisioning(const QString& phase1FastProvisioning);
	QBindable<QString> bindablePhase1FastProvisioning() { return &this->bPhase1FastProvisioning; };
	[[nodiscard]] QString phase1Peaplabel() const { return this->bPhase1Peaplabel; };
	void setPhase1Peaplabel(const QString& phase1Peaplabel);
	QBindable<QString> bindablePhase1Peaplabel() { return &this->bPhase1Peaplabel; };
	[[nodiscard]] QStringList phase2AltsubjectMatches() const { return this->bPhase2AltsubjectMatches; };
	void setPhase2AltsubjectMatches(const QStringList& phase2AltsubjectMatches);
	QBindable<QStringList> bindablePhase2AltsubjectMatches() { return &this->bPhase2AltsubjectMatches; };
	[[nodiscard]] QString phase2Auth() const { return this->bPhase2Auth; };
	void setPhase2Auth(const QString& phase2Auth);
	QBindable<QString> bindablePhase2Auth() { return &this->bPhase2Auth; };
	[[nodiscard]] QString phase2Autheap() const { return this->bPhase2Autheap; };
	void setPhase2Autheap(const QString& phase2Autheap);
	QBindable<QString> bindablePhase2Autheap() { return &this->bPhase2Autheap; };
	[[nodiscard]] QByteArray phase2CaCert() const { return this->bPhase2CaCert; };
	void setPhase2CaCert(const QByteArray& phase2CaCert);
	QBindable<QByteArray> bindablePhase2CaCert() { return &this->bPhase2CaCert; };
	[[nodiscard]] QString phase2CaPath() const { return this->bPhase2CaPath; };
	void setPhase2CaPath(const QString& phase2CaPath);
	QBindable<QString> bindablePhase2CaPath() { return &this->bPhase2CaPath; };
	[[nodiscard]] QByteArray phase2ClientCert() const { return this->bPhase2ClientCert; };
	void setPhase2ClientCert(const QByteArray& phase2ClientCert);
	QBindable<QByteArray> bindablePhase2ClientCert() { return &this->bPhase2ClientCert; };
	[[nodiscard]] QString phase2DomainMatch() const { return this->bPhase2DomainMatch; };
	void setPhase2DomainMatch(const QString& phase2DomainMatch);
	QBindable<QString> bindablePhase2DomainMatch() { return &this->bPhase2DomainMatch; };
	[[nodiscard]] QString phase2DomainSuffixMatch() const { return this->bPhase2DomainSuffixMatch; };
	void setPhase2DomainSuffixMatch(const QString& phase2DomainSuffixMatch);
	QBindable<QString> bindablePhase2DomainSuffixMatch() { return &this->bPhase2DomainSuffixMatch; };
	[[nodiscard]] QByteArray phase2PrivateKey() const { return this->bPhase2PrivateKey; };
	void setPhase2PrivateKey(const QByteArray& phase2PrivateKey);
	QBindable<QByteArray> bindablePhase2PrivateKey() { return &this->bPhase2PrivateKey; };
	[[nodiscard]] QString phase2SubjectMatch() const { return this->bPhase2SubjectMatch; };
	void setPhase2SubjectMatch(const QString& phase2SubjectMatch);
	QBindable<QString> bindablePhase2SubjectMatch() { return &this->bPhase2SubjectMatch; };
	[[nodiscard]] QString subjectMatch() const { return this->bSubjectMatch; };
	void setSubjectMatch(const QString& subjectMatch);
	QBindable<QString> bindableSubjectMatch() { return &this->bSubjectMatch; };
	[[nodiscard]] bool systemCaCerts() const { return this->bSystemCaCerts; };
	void setSystemCaCerts(bool systemCaCerts);
	QBindable<bool> bindableSystemCaCerts() { return &this->bSystemCaCerts; };
	[[nodiscard]] quint32 caCertPasswordFlags() const { return this->bCaCertPasswordFlags; };
	void setCaCertPasswordFlags(quint32 caCertPasswordFlags);
	QBindable<quint32> bindableCaCertPasswordFlags() { return &this->bCaCertPasswordFlags; };
	[[nodiscard]] quint32 clientCertPasswordFlags() const { return this->bClientCertPasswordFlags; };
	void setClientCertPasswordFlags(quint32 clientCertPasswordFlags);
	QBindable<quint32> bindableClientCertPasswordFlags() { return &this->bClientCertPasswordFlags; };
	[[nodiscard]] quint32 passwordFlags() const { return this->bPasswordFlags; };
	void setPasswordFlags(quint32 passwordFlags);
	QBindable<quint32> bindablePasswordFlags() { return &this->bPasswordFlags; };
	[[nodiscard]] quint32 passwordRawFlags() const { return this->bPasswordRawFlags; };
	void setPasswordRawFlags(quint32 passwordRawFlags);
	QBindable<quint32> bindablePasswordRawFlags() { return &this->bPasswordRawFlags; };
	[[nodiscard]] quint32 phase2CaCertPasswordFlags() const { return this->bPhase2CaCertPasswordFlags; };
	void setPhase2CaCertPasswordFlags(quint32 phase2CaCertPasswordFlags);
	QBindable<quint32> bindablePhase2CaCertPasswordFlags() { return &this->bPhase2CaCertPasswordFlags; };
	[[nodiscard]] quint32 phase2ClientCertPasswordFlags() const { return this->bPhase2ClientCertPasswordFlags; };
	void setPhase2ClientCertPasswordFlags(quint32 phase2ClientCertPasswordFlags);
	QBindable<quint32> bindablePhase2ClientCertPasswordFlags() { return &this->bPhase2ClientCertPasswordFlags; };
	[[nodiscard]] quint32 phase2PrivateKeyPasswordFlags() const { return this->bPhase2PrivateKeyPasswordFlags; };
	void setPhase2PrivateKeyPasswordFlags(quint32 phase2PrivateKeyPasswordFlags);
	QBindable<quint32> bindablePhase2PrivateKeyPasswordFlags() { return &this->bPhase2PrivateKeyPasswordFlags; };
	[[nodiscard]] quint32 pinFlags() const { return this->bPinFlags; };
	void setPinFlags(quint32 pinFlags);
	QBindable<quint32> bindablePinFlags() { return &this->bPinFlags; };
	[[nodiscard]] quint32 privateKeyPasswordFlags() const { return this->bPrivateKeyPasswordFlags; };
	void setPrivateKeyPasswordFlags(quint32 privateKeyPasswordFlags);
	QBindable<quint32> bindablePrivateKeyPasswordFlags() { return &this->bPrivateKeyPasswordFlags; };
	// clang-format on

	QBindable<QVariantMap> bindableSettings() { return &this->bSettings; };

	Q_INVOKABLE void setCaCertPassword(const QString& password);
	Q_INVOKABLE void setClientCertPassword(const QString& password);
	Q_INVOKABLE void setPassword(const QString& password);
	Q_INVOKABLE void setPasswordRaw(const QByteArray& password);
	Q_INVOKABLE void setPhase2CaCertPassword(const QString& password);
	Q_INVOKABLE void setPhase2ClientCertPassword(const QString& password);
	Q_INVOKABLE void setPhase2PrivateKeyPassword(const QString& password);
	Q_INVOKABLE void setPin(const QString& pin);
	Q_INVOKABLE void setPrivateKeyPassword(const QString& password);

signals:
	void settingsChanged();
	void altsubjectMatchesChanged();
	void anonymousIdentityChanged();
	void authTimeoutChanged();
	void caCertChanged();
	void caPathChanged();
	void clientCertChanged();
	void domainMatchChanged();
	void eapChanged();
	void identityChanged();
	void opensslCiphersChanged();
	void optionalChanged();
	void pacFileChanged();
	void phase1AuthFlagsChanged();
	void phase1FastProvisioningChanged();
	void phase1PeaplabelChanged();
	void phase2AltsubjectMatchesChanged();
	void phase2AuthChanged();
	void phase2AutheapChanged();
	void phase2CaCertChanged();
	void phase2CaPathChanged();
	void phase2ClientCertChanged();
	void phase2DomainMatchChanged();
	void phase2DomainSuffixMatchChanged();
	void phase2PrivateKeyChanged();
	void phase2SubjectMatchChanged();
	void subjectMatchChanged();
	void systemCaCertsChanged();
	void caCertPasswordFlagsChanged();
	void clientCertPasswordFlagsChanged();
	void passwordFlagsChanged();
	void passwordRawFlagsChanged();
	void phase2CaCertPasswordFlagsChanged();
	void phase2ClientCertPasswordFlagsChanged();
	void phase2PrivateKeyPasswordFlagsChanged();
	void pinFlagsChanged();
	void privateKeyPasswordFlagsChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QVariantMap, bSettings, &NM8021xSettings::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QStringList, bAltsubjectMatches, &NM8021xSettings::altsubjectMatchesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bAnonymousIdentity, &NM8021xSettings::anonymousIdentityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, qint32, bAuthTimeout, &NM8021xSettings::authTimeoutChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QByteArray, bCaCert, &NM8021xSettings::caCertChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bCaPath, &NM8021xSettings::caPathChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QByteArray, bClientCert, &NM8021xSettings::clientCertChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bDomainMatch, &NM8021xSettings::domainMatchChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QStringList, bEap, &NM8021xSettings::eapChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bIdentity, &NM8021xSettings::identityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bOpensslCiphers, &NM8021xSettings::opensslCiphersChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, bool, bOptional, &NM8021xSettings::optionalChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPacFile, &NM8021xSettings::pacFileChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPhase1AuthFlags, &NM8021xSettings::phase1AuthFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase1FastProvisioning, &NM8021xSettings::phase1FastProvisioningChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase1Peaplabel, &NM8021xSettings::phase1PeaplabelChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QStringList, bPhase2AltsubjectMatches, &NM8021xSettings::phase2AltsubjectMatchesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase2Auth, &NM8021xSettings::phase2AuthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase2Autheap, &NM8021xSettings::phase2AutheapChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QByteArray, bPhase2CaCert, &NM8021xSettings::phase2CaCertChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase2CaPath, &NM8021xSettings::phase2CaPathChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QByteArray, bPhase2ClientCert, &NM8021xSettings::phase2ClientCertChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase2DomainMatch, &NM8021xSettings::phase2DomainMatchChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase2DomainSuffixMatch, &NM8021xSettings::phase2DomainSuffixMatchChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QByteArray, bPhase2PrivateKey, &NM8021xSettings::phase2PrivateKeyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bPhase2SubjectMatch, &NM8021xSettings::phase2SubjectMatchChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, QString, bSubjectMatch, &NM8021xSettings::subjectMatchChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, bool, bSystemCaCerts, &NM8021xSettings::systemCaCertsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bCaCertPasswordFlags, &NM8021xSettings::caCertPasswordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bClientCertPasswordFlags, &NM8021xSettings::clientCertPasswordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPasswordFlags, &NM8021xSettings::passwordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPasswordRawFlags, &NM8021xSettings::passwordRawFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPhase2CaCertPasswordFlags, &NM8021xSettings::phase2CaCertPasswordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPhase2ClientCertPasswordFlags, &NM8021xSettings::phase2ClientCertPasswordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPhase2PrivateKeyPasswordFlags, &NM8021xSettings::phase2PrivateKeyPasswordFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPinFlags, &NM8021xSettings::pinFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NM8021xSettings, quint32, bPrivateKeyPasswordFlags, &NM8021xSettings::privateKeyPasswordFlagsChanged);
	// clang-format on
};

// Proxy of a /org/freedesktop/NetworkManager/Settings/Connection/* object.
///! A NetworkManager connection settings profile.
class NMSettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	Q_PROPERTY(NMConnectionSettings* connectionSettings READ connectionSettings BINDABLE bindableConnectionSettings);
	Q_PROPERTY(NMWirelessSettings* wirelessSettings READ wirelessSettings BINDABLE bindableWirelessSettings);
	Q_PROPERTY(NMWirelessSecuritySettings* wirelessSecuritySettings READ wirelessSecuritySettings BINDABLE bindableWirelessSecuritySettings);
	Q_PROPERTY(NM8021xSettings* authSettings READ authSettings BINDABLE bindableAuthSettings);
	// clang-format on

public:
	explicit NMSettings(const QString& path, QObject* parent = nullptr);

	/// Clear all of the secrets belonging to the settings.
	Q_INVOKABLE void clearSecrets();
	/// Delete the settings.
	Q_INVOKABLE void forget();

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	// clang-format off
	[[nodiscard]] NMConnectionSettings* connectionSettings() const { return this->bConnectionSettings; };
	QBindable<NMConnectionSettings*> bindableConnectionSettings() { return &this->bConnectionSettings; };
	[[nodiscard]] NMWirelessSettings* wirelessSettings() const { return this->bWirelessSettings; };
	QBindable<NMWirelessSettings*> bindableWirelessSettings() { return &this->bWirelessSettings; };
	[[nodiscard]] NMWirelessSecuritySettings* wirelessSecuritySettings() const { return this->bWirelessSecuritySettings; };
	QBindable<NMWirelessSecuritySettings*> bindableWirelessSecuritySettings() { return &this->bWirelessSecuritySettings; };
	[[nodiscard]] NM8021xSettings* authSettings() const { return this->bAuthSettings; };
	QBindable<NM8021xSettings*> bindableAuthSettings() { return &this->bAuthSettings; };
	// clang-format on

signals:
	void loaded();
	void connectionSettingsChanged();
	void wirelessSettingsChanged();
	void wirelessSecuritySettingsChanged();
	void authSettingsChanged();
	void settingsChanged(ConnectionSettingsMap settings);

private:
	bool mLoaded = false;
	bool mUpdatePending = false;
	ConnectionSettingsMap mProxySettings;

	void getSettings();
	void updateSettings();
	void queueUpdate();

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, NMConnectionSettings*, bConnectionSettings, &NMSettings::connectionSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, NMWirelessSettings*, bWirelessSettings, &NMSettings::wirelessSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, NMWirelessSecuritySettings*, bWirelessSecuritySettings, &NMSettings::wirelessSecuritySettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, NM8021xSettings*, bAuthSettings, &NMSettings::authSettingsChanged);
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMSettings, connectionSettingsProperties);
	// clang-format on

	DBusNMConnectionSettingsProxy* proxy = nullptr;
};

} // namespace qs::network
