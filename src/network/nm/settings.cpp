#include "settings.hpp"
#include <algorithm>

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qfile.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "dbus_nm_connection_settings.h"
#include "types.hpp"
#include "utils.hpp"

namespace qs::network {
using std::ranges::all_of;

using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);

void setSetting(QBindable<QVariantMap> bSettings, const QString& key, const QVariant& value) {
	auto settings = bSettings.value();
	settings[key] = value;
	bSettings.setValue(settings);
}

bool isValidSecretFlags(quint32 flags) {
	const quint32 allFlags =
	    NMSecretFlag::AgentOwned | NMSecretFlag::NotSaved | NMSecretFlag::NotRequired;
	return (flags & ~allFlags) == 0;
}

} // namespace

NMSettings::NMSettings(const QString& path, QObject* parent): QObject(parent) {
	qDBusRegisterMetaType<ConnectionSettingsMap>();

	this->proxy = new DBusNMConnectionSettingsProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection at" << path;
		return;
	}

	// clang-format off
	QObject::connect(this->proxy, &DBusNMConnectionSettingsProxy::Updated, this, &NMSettings::getSettings);
	// clang-format on

	this->connectionSettingsProperties.setInterface(this->proxy);
	this->connectionSettingsProperties.updateAllViaGetAll();

	this->getSettings();
}

void NMSettings::queueUpdate() {
	if (this->mUpdatePending) return;
	this->mUpdatePending = true;
	QMetaObject::invokeMethod(this, &NMSettings::updateSettings, Qt::QueuedConnection);
}

void NMSettings::getSettings() {
	auto pending = this->proxy->GetSettings();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<ConnectionSettingsMap> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to get settings for" << this->path() << ":" << reply.error().message();
		} else {
			auto settings = reply.value();
			// Source of truth for the settings in NetworkManager.
			// Doesn't update optimistically like the individual props in subsetting classses.
			this->mProxySettings = settings;

			if (settings.contains("connection")) {
				if (!this->bConnectionSettings) {
					auto* conn = new NMConnectionSettings(this);
					// clang-format off
					QObject::connect(conn, &NMConnectionSettings::settingsChanged, this, &NMSettings::queueUpdate);
					// clang-format on
					this->bConnectionSettings = conn;
				}
				this->bConnectionSettings->bindableSettings().setValue(settings["connection"]);
			} else if (this->bConnectionSettings) {
				delete this->bConnectionSettings;
				this->bConnectionSettings = nullptr;
			}

			if (settings.contains("802-11-wireless")) {
				if (!this->bWirelessSettings) {
					auto* wifi = new NMWirelessSettings(this);
					// clang-format off
					QObject::connect(wifi, &NMWirelessSettings::settingsChanged, this, &NMSettings::queueUpdate);
					// clang-format on
					this->bWirelessSettings = wifi;
				}
				this->bWirelessSettings->bindableSettings().setValue(settings["802-11-wireless"]);
			} else if (this->bWirelessSettings) {
				delete this->bWirelessSettings;
				this->bWirelessSettings = nullptr;
			}

			if (settings.contains("802-11-wireless-security")) {
				if (!this->bWirelessSecuritySettings) {
					auto* sec = new NMWirelessSecuritySettings(this);
					// clang-format off
					QObject::connect(sec, &NMWirelessSecuritySettings::settingsChanged, this, &NMSettings::queueUpdate);
					// clang-format on
					this->bWirelessSecuritySettings = sec;
				}
				this->bWirelessSecuritySettings->bindableSettings().setValue(
				    settings["802-11-wireless-security"]
				);
			} else if (this->bWirelessSecuritySettings) {
				delete this->bWirelessSecuritySettings;
				this->bWirelessSecuritySettings = nullptr;
			}

			if (settings.contains("802-1x")) {
				if (!this->bAuthSettings) {
					auto* eap = new NM8021xSettings(this);
					// clang-format off
					QObject::connect(eap, &NM8021xSettings::settingsChanged, this, &NMSettings::queueUpdate);
					// clang-format on
					this->bAuthSettings = eap;
				}
				this->bAuthSettings->bindableSettings().setValue(settings["802-1x"]);
			} else if (this->bAuthSettings) {
				delete this->bAuthSettings;
				this->bAuthSettings = nullptr;
			}
		}

		if (!this->mLoaded) {
			emit this->loaded();
			this->mLoaded = true;
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMSettings::updateSettings() {
	this->mUpdatePending = false;
	ConnectionSettingsMap settings = this->mProxySettings;
	if (this->bConnectionSettings)
		settings["connection"] = this->bConnectionSettings->bindableSettings().value();
	if (this->bWirelessSettings)
		settings["802-11-wireless"] = this->bWirelessSettings->bindableSettings().value();
	if (this->bWirelessSecuritySettings)
		settings["802-11-wireless-security"] =
		    this->bWirelessSecuritySettings->bindableSettings().value();
	if (this->bAuthSettings) settings["802-1x"] = this->bAuthSettings->bindableSettings().value();
	auto pending = this->proxy->Update(settings);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to update settings for" << this->path() << ":" << reply.error().message();
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMSettings::clearSecrets() {
	auto pending = this->proxy->ClearSecrets();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to clear secrets for" << this->path() << ":" << reply.error().message();
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMSettings::forget() {
	auto pending = this->proxy->Delete();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to forget" << this->path() << ":" << reply.error().message();
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

bool NMSettings::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMSettings::address() const { return this->proxy ? this->proxy->service() : QString(); }
QString NMSettings::path() const { return this->proxy ? this->proxy->path() : QString(); }

NMConnectionSettings::NMConnectionSettings(QObject* parent): QObject(parent) {
	// clang-format off
	this->bAutoconnectPriority.setBinding([this]() { return this->bSettings.value().value("autoconnect-priority").toInt(); });
	this->bIpPingTimeout.setBinding([this]() { return this->bSettings.value().value("ip-ping-timeout").toUInt(); });
	this->bMetered.setBinding([this]() { return this->bSettings.value().value("metered").toInt(); });
	this->bMultiConnect.setBinding([this]() { return this->bSettings.value().value("multi-connect").toInt(); });
	this->bTimestamp.setBinding([this]() { return this->bSettings.value().value("timestamp").toULongLong(); });
	this->bZone.setBinding([this]() { return this->bSettings.value().value("zone").toString(); });
	this->bUuid.setBinding([this]() { return this->bSettings.value().value("uuid").toString(); });
	this->bPermissions.setBinding([this]() { return this->bSettings.value().value("permissions").toStringList(); });
	this->bPortType.setBinding([this]() { return this->bSettings.value().value("port-type").toString(); });
	this->bSecondaries.setBinding([this]() { return this->bSettings.value().value("secondaries").toStringList(); });
	this->bStableId.setBinding([this]() { return this->bSettings.value().value("stable-id").toString(); });
	this->bMudUrl.setBinding([this]() { return this->bSettings.value().value("mud-url").toString(); });
	this->bIpPingAddresses.setBinding([this]() { return this->bSettings.value().value("ip-ping-addresses").toStringList(); });
	this->bId.setBinding([this]() { return this->bSettings.value().value("id").toString(); });
	this->bController.setBinding([this]() { return this->bSettings.value().value("controller").toString(); });
	// clang-format on

	// Some NM defaults differ from their type default.
	this->bAutoconnect.setBinding([this]() {
		auto v = this->bSettings.value().value("autoconnect");
		return v.isValid() ? v.toBool() : true;
	});
	this->bAutoconnectPorts.setBinding([this]() {
		auto v = this->bSettings.value().value("autoconnect-ports");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bAutoconnectRetries.setBinding([this]() {
		auto v = this->bSettings.value().value("autoconnect-retries");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bAuthRetries.setBinding([this]() {
		auto v = this->bSettings.value().value("auth-retries");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bDnsOverTls.setBinding([this]() {
		auto v = this->bSettings.value().value("dns-over-tls");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bDnssec.setBinding([this]() {
		auto v = this->bSettings.value().value("dnssec");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bIpPingAddressesRequireAll.setBinding([this]() {
		auto v = this->bSettings.value().value("ip-ping-addresses-require-all");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bLldp.setBinding([this]() {
		auto v = this->bSettings.value().value("lldp");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bLlmnr.setBinding([this]() {
		auto v = this->bSettings.value().value("llmnr");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bMdns.setBinding([this]() {
		auto v = this->bSettings.value().value("mdns");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bWaitActivationDelay.setBinding([this]() {
		auto v = this->bSettings.value().value("wait-activation-delay");
		return v.isValid() ? v.toInt() : -1;
	});
	this->bWaitDeviceTimeout.setBinding([this]() {
		auto v = this->bSettings.value().value("wait-device-timeout");
		return v.isValid() ? v.toInt() : -1;
	});
}

void NMConnectionSettings::setId(const QString& id) {
	if (this->id() == id) return;
	setSetting(this->bindableSettings(), "id", id);
}

void NMConnectionSettings::setAutoconnect(bool autoconnect) {
	if (this->autoconnect() == autoconnect) return;
	setSetting(this->bindableSettings(), "autoconnect", autoconnect);
}

void NMConnectionSettings::setAutoconnectPriority(qint32 autoconnectPriority) {
	if (this->autoconnectPriority() == autoconnectPriority) return;
	setSetting(this->bindableSettings(), "autoconnect-priority", autoconnectPriority);
}

void NMConnectionSettings::setAutoconnectPorts(qint32 autoconnectPorts) {
	if (this->autoconnectPorts() == autoconnectPorts) return;
	if (autoconnectPorts < -1 || autoconnectPorts > 1) return;
	setSetting(this->bindableSettings(), "autoconnect-ports", autoconnectPorts);
}

void NMConnectionSettings::setAutoconnectRetries(qint32 autoconnectRetries) {
	if (this->autoconnectRetries() == autoconnectRetries) return;
	if (autoconnectRetries < -1) return;
	setSetting(this->bindableSettings(), "autoconnect-retries", autoconnectRetries);
}

void NMConnectionSettings::setAuthRetries(qint32 authRetries) {
	if (this->authRetries() == authRetries) return;
	if (authRetries < -1) return;
	setSetting(this->bindableSettings(), "auth-retries", authRetries);
}

void NMConnectionSettings::setController(const QString& controller) {
	if (this->controller() == controller) return;
	setSetting(this->bindableSettings(), "controller", controller);
}

void NMConnectionSettings::setDnsOverTls(qint32 dnsOverTls) {
	if (this->dnsOverTls() == dnsOverTls) return;
	if (dnsOverTls < -1 || dnsOverTls > 2) return;
	setSetting(this->bindableSettings(), "dns-over-tls", dnsOverTls);
}

void NMConnectionSettings::setDnssec(qint32 dnssec) {
	if (this->dnssec() == dnssec) return;
	if (dnssec < -1 || dnssec > 2) return;
	setSetting(this->bindableSettings(), "dnssec", dnssec);
}

void NMConnectionSettings::setIpPingAddresses(const QStringList& ipPingAddresses) {
	if (this->ipPingAddresses() == ipPingAddresses) return;
	setSetting(this->bindableSettings(), "ip-ping-addresses", QVariant::fromValue(ipPingAddresses));
}

void NMConnectionSettings::setIpPingAddressesRequireAll(qint32 ipPingAddressesRequireAll) {
	if (this->ipPingAddressesRequireAll() == ipPingAddressesRequireAll) return;
	if (ipPingAddressesRequireAll < -1 || ipPingAddressesRequireAll > 1) return;
	setSetting(this->bindableSettings(), "ip-ping-addresses-require-all", ipPingAddressesRequireAll);
}

void NMConnectionSettings::setIpPingTimeout(quint32 ipPingTimeout) {
	if (this->ipPingTimeout() == ipPingTimeout) return;
	setSetting(this->bindableSettings(), "ip-ping-timeout", ipPingTimeout);
}

void NMConnectionSettings::setLldp(qint32 lldp) {
	if (this->lldp() == lldp) return;
	if (lldp < -1 || lldp > 1) return;
	setSetting(this->bindableSettings(), "lldp", lldp);
}

void NMConnectionSettings::setLlmnr(qint32 llmnr) {
	if (this->llmnr() == llmnr) return;
	if (llmnr < -1 || llmnr > 2) return;
	setSetting(this->bindableSettings(), "llmnr", llmnr);
}

void NMConnectionSettings::setMdns(qint32 mdns) {
	if (this->mdns() == mdns) return;
	if (mdns < -1 || mdns > 2) return;
	setSetting(this->bindableSettings(), "mdns", mdns);
}

void NMConnectionSettings::setMetered(qint32 metered) {
	if (this->metered() == metered) return;
	if (metered < 0 || metered > 1) return;
	setSetting(this->bindableSettings(), "metered", metered);
}

void NMConnectionSettings::setMudUrl(const QString& mudUrl) {
	if (this->mudUrl() == mudUrl) return;
	if (mudUrl != "none") {
		auto url = QUrl(mudUrl);
		if (!url.isValid() || url.scheme() != "https") return;
	}
	setSetting(this->bindableSettings(), "mud-url", mudUrl);
}

void NMConnectionSettings::setMultiConnect(qint32 multiConnect) {
	if (this->multiConnect() == multiConnect) return;
	if (multiConnect < 0 || multiConnect > 3) return;
	setSetting(this->bindableSettings(), "multi-connect", multiConnect);
}

void NMConnectionSettings::setPermissions(const QStringList& permissions) {
	if (this->permissions() == permissions) return;
	setSetting(this->bindableSettings(), "permissions", QVariant::fromValue(permissions));
}

void NMConnectionSettings::setPortType(const QString& portType) {
	if (this->portType() == portType) return;
	setSetting(this->bindableSettings(), "port-type", portType);
}

void NMConnectionSettings::setSecondaries(const QStringList& secondaries) {
	if (this->secondaries() == secondaries) return;
	setSetting(this->bindableSettings(), "secondaries", QVariant::fromValue(secondaries));
}

void NMConnectionSettings::setStableId(const QString& stableId) {
	if (this->stableId() == stableId) return;
	setSetting(this->bindableSettings(), "stable-id", stableId);
}

void NMConnectionSettings::setWaitActivationDelay(qint32 waitActivationDelay) {
	if (this->waitActivationDelay() == waitActivationDelay) return;
	if (waitActivationDelay < -1) return;
	setSetting(this->bindableSettings(), "wait-activation-delay", waitActivationDelay);
}

void NMConnectionSettings::setWaitDeviceTimeout(qint32 waitDeviceTimeout) {
	if (this->waitDeviceTimeout() == waitDeviceTimeout) return;
	if (waitDeviceTimeout < -1) return;
	setSetting(this->bindableSettings(), "wait-device-timeout", waitDeviceTimeout);
}

void NMConnectionSettings::setZone(const QString& zone) {
	if (this->zone() == zone) return;
	setSetting(this->bindableSettings(), "zone", zone);
}

NMWirelessSettings::NMWirelessSettings(QObject* parent): QObject(parent) {
	// clang-format off
	this->bSsid.setBinding([this]() { return this->bSettings.value().value("ssid").toByteArray(); });
	this->bSeenBssids.setBinding([this]() { return this->bSettings.value().value("seen-bssids").toStringList(); });
	this->bMode.setBinding([this]() { return this->bSettings.value().value("mode").toString(); });
	this->bBand.setBinding([this]() { return this->bSettings.value().value("band").toString(); });
	this->bAssignedMacAddress.setBinding([this]() { return this->bSettings.value().value("assigned-mac-address").toString(); });
	this->bGenerateMacAddressMask.setBinding([this]() { return this->bSettings.value().value("generate-mac-address-mask").toString(); });
	this->bMacAddress.setBinding([this]() { return this->bSettings.value().value("mac-address").toByteArray(); });
	this->bMacAddressBlacklist.setBinding([this]() { return this->bSettings.value().value("mac-address-blacklist").toStringList(); });
	this->bMacAddressRandomization.setBinding([this]() { return this->bSettings.value().value("mac-address-randomization").toUInt(); });
	this->bBssid.setBinding([this]() { return this->bSettings.value().value("bssid").toByteArray(); });
	this->bChannel.setBinding([this]() { return this->bSettings.value().value("channel").toUInt(); });
	this->bChannelWidth.setBinding([this]() { return this->bSettings.value().value("channel-width").toInt(); });
	this->bHidden.setBinding([this]() { return this->bSettings.value().value("hidden").toBool(); });
	this->bPowersave.setBinding([this]() { return this->bSettings.value().value("powersave").toUInt(); });
	this->bMtu.setBinding([this]() { return this->bSettings.value().value("mtu").toUInt(); });
	// clang-format on

	this->bApIsolation.setBinding([this]() {
		auto v = this->bSettings.value().value("ap-isolation");
		return v.isValid() ? v.toInt() : -1;
	});
}

void NMWirelessSettings::setMacAddressRandomization(quint32 macAddressRandomization) {
	if (this->macAddressRandomization() == macAddressRandomization) return;
	if (macAddressRandomization > 2) return;
	setSetting(this->bindableSettings(), "mac-address-randomization", macAddressRandomization);
}

void NMWirelessSettings::setMacAddressBlacklist(const QStringList& macAddressBlacklist) {
	if (this->macAddressBlacklist() == macAddressBlacklist) return;
	setSetting(
	    this->bindableSettings(),
	    "mac-address-blacklist",
	    QVariant::fromValue(macAddressBlacklist)
	);
}

void NMWirelessSettings::setMacAddress(const QByteArray& macAddress) {
	if (this->macAddress() == macAddress) return;
	setSetting(this->bindableSettings(), "mac-address", macAddress);
}

void NMWirelessSettings::setGenerateMacAddressMask(const QString& generateMacAddressMask) {
	if (this->generateMacAddressMask() == generateMacAddressMask) return;
	setSetting(this->bindableSettings(), "generate-mac-address-mask", generateMacAddressMask);
}

void NMWirelessSettings::setAssignedMacAddress(const QString& assignedMacAddress) {
	if (this->assignedMacAddress() == assignedMacAddress) return;
	setSetting(this->bindableSettings(), "assigned-mac-address", assignedMacAddress);
}

void NMWirelessSettings::setHidden(bool hidden) {
	if (this->hidden() == hidden) return;
	setSetting(this->bindableSettings(), "hidden", hidden);
}

void NMWirelessSettings::setChannelWidth(qint32 channelWidth) {
	if (this->channelWidth() == channelWidth) return;
	setSetting(this->bindableSettings(), "channel-width", channelWidth);
}

void NMWirelessSettings::setChannel(quint32 channel) {
	if (this->channel() == channel) return;
	setSetting(this->bindableSettings(), "channel", channel);
}

void NMWirelessSettings::setBssid(const QByteArray& bssid) {
	if (this->bssid() == bssid) return;
	setSetting(this->bindableSettings(), "bssid", bssid);
}

void NMWirelessSettings::setBand(const QString& band) {
	if (this->band() == band) return;
	if (band != "a" && band != "bg") return;
	setSetting(this->bindableSettings(), "band", band);
}

void NMWirelessSettings::setApIsolation(qint32 apIsolation) {
	if (this->apIsolation() == apIsolation) return;
	if (this->mode() != "ap") return;
	setSetting(this->bindableSettings(), "ap-isolation", apIsolation);
}

void NMWirelessSettings::setMtu(quint32 mtu) {
	if (this->mtu() == mtu) return;
	setSetting(this->bindableSettings(), "mtu", mtu);
}

void NMWirelessSettings::setPowersave(quint32 powersave) {
	if (this->powersave() == powersave) return;
	if (powersave > 3) return;
	setSetting(this->bindableSettings(), "powersave", powersave);
}

NMWirelessSecuritySettings::NMWirelessSecuritySettings(QObject* parent): QObject(parent) {
	// clang-format off
	this->bAuthAlg.setBinding([this]() { return this->bSettings.value().value("auth-alg").toString(); });
	this->bFils.setBinding([this]() { return this->bSettings.value().value("fils").toInt(); });
	this->bGroup.setBinding([this]() { return this->bSettings.value().value("group").toStringList(); });
	this->bKeyMgmt.setBinding([this]() { return this->bSettings.value().value("key-mgmt").toString(); });
	this->bLeapUsername.setBinding([this]() { return this->bSettings.value().value("leap-username").toString(); });
	this->bPairwise.setBinding([this]() { return this->bSettings.value().value("pairwise").toStringList(); });
	this->bPmf.setBinding([this]() { return this->bSettings.value().value("pmf").toInt(); });
	this->bProto.setBinding([this]() { return this->bSettings.value().value("proto").toStringList(); });
	this->bWepKeyType.setBinding([this]() { return this->bSettings.value().value("wep-key-type").toUInt(); });
	this->bWpsMethod.setBinding([this]() { return this->bSettings.value().value("wps-method").toUInt(); });
	this->bLeapPasswordFlags.setBinding([this]() { return this->bSettings.value().value("leap-password-flags").toUInt(); });
	this->bPskFlags.setBinding([this]() { return this->bSettings.value().value("psk-flags").toUInt(); });
	this->bWepKeyFlags.setBinding([this]() { return this->bSettings.value().value("wep-key-flags").toUInt(); });
	// clang-format on
	this->bSecurity.setBinding([this]() {
		return securityFromWifiSettings(this->bKeyMgmt, this->bAuthAlg, this->bProto);
	});
}

void NMWirelessSecuritySettings::setKeyMgmt(const QString& keyMgmt) {
	if (this->keyMgmt() == keyMgmt) return;
	setSetting(this->bindableSettings(), "key-mgmt", keyMgmt);
}

void NMWirelessSecuritySettings::setGroup(const QStringList& group) {
	if (this->group() == group) return;
	setSetting(this->bindableSettings(), "group", QVariant::fromValue(group));
}

void NMWirelessSecuritySettings::setFils(qint32 fils) {
	if (this->fils() == fils) return;
	setSetting(this->bindableSettings(), "fils", fils);
}

void NMWirelessSecuritySettings::setAuthAlg(const QString& authAlg) {
	if (this->authAlg() == authAlg) return;
	setSetting(this->bindableSettings(), "auth-alg", authAlg);
}

void NMWirelessSecuritySettings::setLeapUsername(const QString& leapUsername) {
	if (this->leapUsername() == leapUsername) return;
	setSetting(this->bindableSettings(), "leap-username", leapUsername);
}

void NMWirelessSecuritySettings::setPairwise(const QStringList& pairwise) {
	if (this->pairwise() == pairwise) return;
	setSetting(this->bindableSettings(), "pairwise", QVariant::fromValue(pairwise));
}

void NMWirelessSecuritySettings::setPmf(qint32 pmf) {
	if (this->pmf() == pmf) return;
	setSetting(this->bindableSettings(), "pmf", pmf);
}

void NMWirelessSecuritySettings::setProto(const QStringList& proto) {
	if (this->proto() == proto) return;
	setSetting(this->bindableSettings(), "proto", QVariant::fromValue(proto));
}

void NMWirelessSecuritySettings::setWepKeyType(quint32 wepKeyType) {
	if (this->wepKeyType() == wepKeyType) return;
	setSetting(this->bindableSettings(), "wep-key-type", wepKeyType);
}

void NMWirelessSecuritySettings::setWpsMethod(quint32 wpsMethod) {
	if (this->wpsMethod() == wpsMethod) return;
	setSetting(this->bindableSettings(), "wps-method", wpsMethod);
}

void NMWirelessSecuritySettings::setLeapPasswordFlags(quint32 leapPasswordFlags) {
	if (this->leapPasswordFlags() == leapPasswordFlags) return;
	if (!isValidSecretFlags(leapPasswordFlags)) return;
	setSetting(this->bindableSettings(), "leap-password-flags", leapPasswordFlags);
}

void NMWirelessSecuritySettings::setPskFlags(quint32 pskFlags) {
	if (this->pskFlags() == pskFlags) return;
	if (!isValidSecretFlags(pskFlags)) return;
	setSetting(this->bindableSettings(), "psk-flags", pskFlags);
}

void NMWirelessSecuritySettings::setWepKeyFlags(quint32 wepKeyFlags) {
	if (this->wepKeyFlags() == wepKeyFlags) return;
	if (!isValidSecretFlags(wepKeyFlags)) return;
	setSetting(this->bindableSettings(), "wep-key-flags", wepKeyFlags);
}

void NMWirelessSecuritySettings::setPsk(const QString& psk) {
	if (psk.isEmpty()) return;
	const auto psklen = psk.length();
	if (psklen < 8 || psklen > 64) return;
	if (psklen == 64) {
		for (int i = 0; i < psklen; ++i) {
			if (!psk.at(i).isLetterOrNumber()) return;
		}
	}
	setSetting(this->bindableSettings(), "psk", psk);
}

void NMWirelessSecuritySettings::setWepKey(const QString& key, quint32 index, quint32 type) {
	if (index > 3) return;
	if (type == 1) {
		const auto len = key.length();
		const auto isHex =
		    all_of(
		        key,
		        [](QChar c) { return c.isDigit() || (c.toLower() >= 'a' && c.toLower() <= 'f'); }
		    )
		    && (len == 10 || len == 26);
		const auto isAscii = len == 5 || len == 13;
		if (!isHex && !isAscii) return;
	} else if (type == 2) {
		if (key.isEmpty()) return;
	} else {
		return;
	}
	setSetting(this->bindableSettings(), QStringLiteral("wep-key%1").arg(index), key);
	setSetting(this->bindableSettings(), "wep-tx-keyidx", index);
	setSetting(this->bindableSettings(), "wep-key-type", type);
}

void NMWirelessSecuritySettings::setLeapPassword(const QString& password) {
	setSetting(this->bindableSettings(), "leap-password", password);
}

NM8021xSettings::NM8021xSettings(QObject* parent): QObject(parent) {
	// clang-format off
	this->bAltsubjectMatches.setBinding([this]() { return this->bSettings.value().value("altsubject-matches").toStringList(); });
	this->bAnonymousIdentity.setBinding([this]() { return this->bSettings.value().value("anonymous-identity").toString(); });
	this->bAuthTimeout.setBinding([this]() { return this->bSettings.value().value("auth-timeout").toInt(); });
	this->bCaCert.setBinding([this]() { return this->bSettings.value().value("ca-cert").toByteArray(); });
	this->bCaPath.setBinding([this]() { return this->bSettings.value().value("ca-path").toString(); });
	this->bClientCert.setBinding([this]() { return this->bSettings.value().value("client-cert").toByteArray(); });
	this->bDomainMatch.setBinding([this]() { return this->bSettings.value().value("domain-match").toString(); });
	this->bEap.setBinding([this]() { return this->bSettings.value().value("eap").toStringList(); });
	this->bIdentity.setBinding([this]() { return this->bSettings.value().value("identity").toString(); });
	this->bOpensslCiphers.setBinding([this]() { return this->bSettings.value().value("openssl-ciphers").toString(); });
	this->bOptional.setBinding([this]() { return this->bSettings.value().value("optional").toBool(); });
	this->bPacFile.setBinding([this]() { return this->bSettings.value().value("pac-file").toString(); });
	this->bPhase1AuthFlags.setBinding([this]() { return this->bSettings.value().value("phase1-auth-flags").toUInt(); });
	this->bPhase1FastProvisioning.setBinding([this]() { return this->bSettings.value().value("phase1-fast-provisioning").toString(); });
	this->bPhase1Peaplabel.setBinding([this]() { return this->bSettings.value().value("phase1-peaplabel").toString(); });
	this->bPhase2AltsubjectMatches.setBinding([this]() { return this->bSettings.value().value("phase2-altsubject-matches").toStringList(); });
	this->bPhase2Auth.setBinding([this]() { return this->bSettings.value().value("phase2-auth").toString(); });
	this->bPhase2Autheap.setBinding([this]() { return this->bSettings.value().value("phase2-autheap").toString(); });
	this->bPhase2CaCert.setBinding([this]() { return this->bSettings.value().value("phase2-ca-cert").toByteArray(); });
	this->bPhase2CaPath.setBinding([this]() { return this->bSettings.value().value("phase2-ca-path").toString(); });
	this->bPhase2ClientCert.setBinding([this]() { return this->bSettings.value().value("phase2-client-cert").toByteArray(); });
	this->bPhase2DomainMatch.setBinding([this]() { return this->bSettings.value().value("phase2-domain-match").toString(); });
	this->bPhase2DomainSuffixMatch.setBinding([this]() { return this->bSettings.value().value("phase2-domain-suffix-match").toString(); });
	this->bPhase2PrivateKey.setBinding([this]() { return this->bSettings.value().value("phase2-private-key").toByteArray(); });
	this->bPhase2SubjectMatch.setBinding([this]() { return this->bSettings.value().value("phase2-subject-match").toString(); });
	this->bSubjectMatch.setBinding([this]() { return this->bSettings.value().value("subject-match").toString(); });
	this->bSystemCaCerts.setBinding([this]() { return this->bSettings.value().value("system-ca-certs").toBool(); });
	this->bCaCertPasswordFlags.setBinding([this]() { return this->bSettings.value().value("ca-cert-password-flags").toUInt(); });
	this->bClientCertPasswordFlags.setBinding([this]() { return this->bSettings.value().value("client-cert-password-flags").toUInt(); });
	this->bPasswordFlags.setBinding([this]() { return this->bSettings.value().value("password-flags").toUInt(); });
	this->bPasswordRawFlags.setBinding([this]() { return this->bSettings.value().value("password-raw-flags").toUInt(); });
	this->bPhase2CaCertPasswordFlags.setBinding([this]() { return this->bSettings.value().value("phase2-ca-cert-password-flags").toUInt(); });
	this->bPhase2ClientCertPasswordFlags.setBinding([this]() { return this->bSettings.value().value("phase2-client-cert-password-flags").toUInt(); });
	this->bPhase2PrivateKeyPasswordFlags.setBinding([this]() { return this->bSettings.value().value("phase2-private-key-password-flags").toUInt(); });
	this->bPinFlags.setBinding([this]() { return this->bSettings.value().value("pin-flags").toUInt(); });
	this->bPrivateKeyPasswordFlags.setBinding([this]() { return this->bSettings.value().value("private-key-password-flags").toUInt(); });
	// clang-format on
}

void NM8021xSettings::setAltsubjectMatches(const QStringList& altsubjectMatches) {
	if (this->altsubjectMatches() == altsubjectMatches) return;
	setSetting(
	    this->bindableSettings(),
	    "altsubject-matches",
	    QVariant::fromValue(altsubjectMatches)
	);
}

void NM8021xSettings::setAnonymousIdentity(const QString& anonymousIdentity) {
	if (this->anonymousIdentity() == anonymousIdentity) return;
	setSetting(this->bindableSettings(), "anonymous-identity", anonymousIdentity);
}

void NM8021xSettings::setAuthTimeout(qint32 authTimeout) {
	if (this->authTimeout() == authTimeout) return;
	setSetting(this->bindableSettings(), "auth-timeout", authTimeout);
}

void NM8021xSettings::setCaCert(const QByteArray& caCert) {
	if (this->caCert() == caCert) return;
	setSetting(this->bindableSettings(), "ca-cert", caCert);
}

void NM8021xSettings::setCaPath(const QString& caPath) {
	if (this->caPath() == caPath) return;
	setSetting(this->bindableSettings(), "ca-path", caPath);
}

void NM8021xSettings::setClientCert(const QByteArray& clientCert) {
	if (this->clientCert() == clientCert) return;
	setSetting(this->bindableSettings(), "client-cert", clientCert);
}

void NM8021xSettings::setDomainMatch(const QString& domainMatch) {
	if (this->domainMatch() == domainMatch) return;
	setSetting(this->bindableSettings(), "domain-match", domainMatch);
}

void NM8021xSettings::setEap(const QStringList& eap) {
	if (this->eap() == eap) return;
	static const QStringList validMethods = {"leap", "md5", "tls", "peap", "ttls", "pwd", "fast"};
	for (const auto& method: eap) {
		if (!validMethods.contains(method)) return;
	}
	setSetting(this->bindableSettings(), "eap", QVariant::fromValue(eap));
}

void NM8021xSettings::setIdentity(const QString& identity) {
	if (this->identity() == identity) return;
	setSetting(this->bindableSettings(), "identity", identity);
}

void NM8021xSettings::setOpensslCiphers(const QString& opensslCiphers) {
	if (this->opensslCiphers() == opensslCiphers) return;
	setSetting(this->bindableSettings(), "openssl-ciphers", opensslCiphers);
}

void NM8021xSettings::setOptional(bool optional) {
	if (this->optional() == optional) return;
	setSetting(this->bindableSettings(), "optional", optional);
}

void NM8021xSettings::setPacFile(const QString& pacFile) {
	if (this->pacFile() == pacFile) return;
	if (!pacFile.isEmpty() && !QFile::exists(pacFile)) return;
	setSetting(this->bindableSettings(), "pac-file", pacFile);
}

void NM8021xSettings::setPhase1AuthFlags(quint32 phase1AuthFlags) {
	if (this->phase1AuthFlags() == phase1AuthFlags) return;
	setSetting(this->bindableSettings(), "phase1-auth-flags", phase1AuthFlags);
}

void NM8021xSettings::setPhase1FastProvisioning(const QString& phase1FastProvisioning) {
	if (this->phase1FastProvisioning() == phase1FastProvisioning) return;
	bool ok = false;
	auto val = phase1FastProvisioning.toInt(&ok);
	if (!ok || val < 0 || val > 3) return;
	setSetting(this->bindableSettings(), "phase1-fast-provisioning", phase1FastProvisioning);
}

void NM8021xSettings::setPhase1Peaplabel(const QString& phase1Peaplabel) {
	if (this->phase1Peaplabel() == phase1Peaplabel) return;
	setSetting(this->bindableSettings(), "phase1-peaplabel", phase1Peaplabel);
}

void NM8021xSettings::setPhase2AltsubjectMatches(const QStringList& phase2AltsubjectMatches) {
	if (this->phase2AltsubjectMatches() == phase2AltsubjectMatches) return;
	setSetting(
	    this->bindableSettings(),
	    "phase2-altsubject-matches",
	    QVariant::fromValue(phase2AltsubjectMatches)
	);
}

void NM8021xSettings::setPhase2Auth(const QString& phase2Auth) {
	if (this->phase2Auth() == phase2Auth) return;
	setSetting(this->bindableSettings(), "phase2-auth", phase2Auth);
}

void NM8021xSettings::setPhase2Autheap(const QString& phase2Autheap) {
	if (this->phase2Autheap() == phase2Autheap) return;
	setSetting(this->bindableSettings(), "phase2-autheap", phase2Autheap);
}

void NM8021xSettings::setPhase2CaCert(const QByteArray& phase2CaCert) {
	if (this->phase2CaCert() == phase2CaCert) return;
	setSetting(this->bindableSettings(), "phase2-ca-cert", phase2CaCert);
}

void NM8021xSettings::setPhase2CaPath(const QString& phase2CaPath) {
	if (this->phase2CaPath() == phase2CaPath) return;
	setSetting(this->bindableSettings(), "phase2-ca-path", phase2CaPath);
}

void NM8021xSettings::setPhase2ClientCert(const QByteArray& phase2ClientCert) {
	if (this->phase2ClientCert() == phase2ClientCert) return;
	setSetting(this->bindableSettings(), "phase2-client-cert", phase2ClientCert);
}

void NM8021xSettings::setPhase2DomainMatch(const QString& phase2DomainMatch) {
	if (this->phase2DomainMatch() == phase2DomainMatch) return;
	setSetting(this->bindableSettings(), "phase2-domain-match", phase2DomainMatch);
}

void NM8021xSettings::setPhase2DomainSuffixMatch(const QString& phase2DomainSuffixMatch) {
	if (this->phase2DomainSuffixMatch() == phase2DomainSuffixMatch) return;
	setSetting(this->bindableSettings(), "phase2-domain-suffix-match", phase2DomainSuffixMatch);
}

void NM8021xSettings::setPhase2PrivateKey(const QByteArray& phase2PrivateKey) {
	if (this->phase2PrivateKey() == phase2PrivateKey) return;
	setSetting(this->bindableSettings(), "phase2-private-key", phase2PrivateKey);
}

void NM8021xSettings::setPhase2SubjectMatch(const QString& phase2SubjectMatch) {
	if (this->phase2SubjectMatch() == phase2SubjectMatch) return;
	setSetting(this->bindableSettings(), "phase2-subject-match", phase2SubjectMatch);
}

void NM8021xSettings::setSubjectMatch(const QString& subjectMatch) {
	if (this->subjectMatch() == subjectMatch) return;
	setSetting(this->bindableSettings(), "subject-match", subjectMatch);
}

void NM8021xSettings::setSystemCaCerts(bool systemCaCerts) {
	if (this->systemCaCerts() == systemCaCerts) return;
	setSetting(this->bindableSettings(), "system-ca-certs", systemCaCerts);
}

void NM8021xSettings::setCaCertPasswordFlags(quint32 caCertPasswordFlags) {
	if (this->caCertPasswordFlags() == caCertPasswordFlags) return;
	if (!isValidSecretFlags(caCertPasswordFlags)) return;
	setSetting(this->bindableSettings(), "ca-cert-password-flags", caCertPasswordFlags);
}

void NM8021xSettings::setClientCertPasswordFlags(quint32 clientCertPasswordFlags) {
	if (this->clientCertPasswordFlags() == clientCertPasswordFlags) return;
	if (!isValidSecretFlags(clientCertPasswordFlags)) return;
	setSetting(this->bindableSettings(), "client-cert-password-flags", clientCertPasswordFlags);
}

void NM8021xSettings::setPasswordFlags(quint32 passwordFlags) {
	if (this->passwordFlags() == passwordFlags) return;
	if (!isValidSecretFlags(passwordFlags)) return;
	setSetting(this->bindableSettings(), "password-flags", passwordFlags);
}

void NM8021xSettings::setPasswordRawFlags(quint32 passwordRawFlags) {
	if (this->passwordRawFlags() == passwordRawFlags) return;
	if (!isValidSecretFlags(passwordRawFlags)) return;
	setSetting(this->bindableSettings(), "password-raw-flags", passwordRawFlags);
}

void NM8021xSettings::setPhase2CaCertPasswordFlags(quint32 phase2CaCertPasswordFlags) {
	if (this->phase2CaCertPasswordFlags() == phase2CaCertPasswordFlags) return;
	if (!isValidSecretFlags(phase2CaCertPasswordFlags)) return;
	setSetting(this->bindableSettings(), "phase2-ca-cert-password-flags", phase2CaCertPasswordFlags);
}

void NM8021xSettings::setPhase2ClientCertPasswordFlags(quint32 phase2ClientCertPasswordFlags) {
	if (this->phase2ClientCertPasswordFlags() == phase2ClientCertPasswordFlags) return;
	if (!isValidSecretFlags(phase2ClientCertPasswordFlags)) return;
	setSetting(
	    this->bindableSettings(),
	    "phase2-client-cert-password-flags",
	    phase2ClientCertPasswordFlags
	);
}

void NM8021xSettings::setPhase2PrivateKeyPasswordFlags(quint32 phase2PrivateKeyPasswordFlags) {
	if (this->phase2PrivateKeyPasswordFlags() == phase2PrivateKeyPasswordFlags) return;
	if (!isValidSecretFlags(phase2PrivateKeyPasswordFlags)) return;
	setSetting(
	    this->bindableSettings(),
	    "phase2-private-key-password-flags",
	    phase2PrivateKeyPasswordFlags
	);
}

void NM8021xSettings::setPinFlags(quint32 pinFlags) {
	if (this->pinFlags() == pinFlags) return;
	if (!isValidSecretFlags(pinFlags)) return;
	setSetting(this->bindableSettings(), "pin-flags", pinFlags);
}

void NM8021xSettings::setPrivateKeyPasswordFlags(quint32 privateKeyPasswordFlags) {
	if (this->privateKeyPasswordFlags() == privateKeyPasswordFlags) return;
	if (!isValidSecretFlags(privateKeyPasswordFlags)) return;
	setSetting(this->bindableSettings(), "private-key-password-flags", privateKeyPasswordFlags);
}

void NM8021xSettings::setCaCertPassword(const QString& password) {
	setSetting(this->bindableSettings(), "ca-cert-password", password);
}

void NM8021xSettings::setClientCertPassword(const QString& password) {
	setSetting(this->bindableSettings(), "client-cert-password", password);
}

void NM8021xSettings::setPassword(const QString& password) {
	setSetting(this->bindableSettings(), "password", password);
}

void NM8021xSettings::setPasswordRaw(const QByteArray& password) {
	setSetting(this->bindableSettings(), "password-raw", password);
}

void NM8021xSettings::setPhase2CaCertPassword(const QString& password) {
	setSetting(this->bindableSettings(), "phase2-ca-cert-password", password);
}

void NM8021xSettings::setPhase2ClientCertPassword(const QString& password) {
	setSetting(this->bindableSettings(), "phase2-client-cert-password", password);
}

void NM8021xSettings::setPhase2PrivateKeyPassword(const QString& password) {
	setSetting(this->bindableSettings(), "phase2-private-key-password", password);
}

void NM8021xSettings::setPin(const QString& pin) {
	setSetting(this->bindableSettings(), "pin", pin);
}

void NM8021xSettings::setPrivateKeyPassword(const QString& password) {
	setSetting(this->bindableSettings(), "private-key-password", password);
}

} // namespace qs::network
