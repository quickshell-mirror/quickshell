#include "connection.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "dbus_nm_active_connection.h"
#include "dbus_nm_connection_settings.h"
#include "enums.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMConnectionSettings::NMConnectionSettings(const QString& path, QObject* parent): QObject(parent) {
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
	QObject::connect(this->proxy, &DBusNMConnectionSettingsProxy::Updated, this, &NMConnectionSettings::getSettings);
	// clang-format on

	this->bSecurity.setBinding([this]() { return securityFromConnectionSettings(this->bSettings); });
	this->bId.setBinding([this]() {
		return this->bSettings.value().value("connection").value("id").toString();
	});

	this->connectionSettingsProperties.setInterface(this->proxy);
	this->connectionSettingsProperties.updateAllViaGetAll();

	this->getSettings();
}

void NMConnectionSettings::getSettings() {
	auto pending = this->proxy->GetSettings();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<ConnectionSettingsMap> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to get settings for" << this->path() << ":" << reply.error().message();
		} else {
			this->bSettings = reply.value();
		}

		if (!this->mLoaded) {
			emit this->loaded();
			this->mLoaded = true;
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMConnectionSettings::setWifiPsk(const QString& psk) {
	QVariantMap wifiSecurity;
	wifiSecurity["psk"] = psk;
	this->updateSettings("802-11-wireless-security", wifiSecurity);
}

void NMConnectionSettings::updateSettings(const QString& mapName, const QVariantMap& map) {
	ConnectionSettingsMap newSettings;
	newSettings.insert(mapName, map);

	auto merged = mergeSettingsMaps(this->bSettings, newSettings);
	auto pending = this->proxy->Update(merged);
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

void NMConnectionSettings::clearSecrets() {
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

void NMConnectionSettings::forget() {
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

bool NMConnectionSettings::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMConnectionSettings::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMConnectionSettings::path() const { return this->proxy ? this->proxy->path() : QString(); }

NMActiveConnection::NMActiveConnection(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMActiveConnectionProxy(
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
	QObject::connect(&this->activeConnectionProperties, &DBusPropertyGroup::getAllFinished, this, &NMActiveConnection::loaded, Qt::SingleShotConnection);
	QObject::connect(this->proxy, &DBusNMActiveConnectionProxy::StateChanged, this, &NMActiveConnection::onStateChanged);
	// clang-format on

	this->activeConnectionProperties.setInterface(this->proxy);
	this->activeConnectionProperties.updateAllViaGetAll();
}

void NMActiveConnection::onStateChanged(quint32 /*state*/, quint32 reason) {
	auto enumReason = static_cast<NMNetworkStateReason::Enum>(reason);
	if (this->bStateReason == enumReason) return;
	this->bStateReason = enumReason;
}

bool NMActiveConnection::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMActiveConnection::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMActiveConnection::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMConnectionState::Enum>
DBusDataTransform<qs::network::NMConnectionState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMConnectionState::Enum>(wire));
}

} // namespace qs::dbus
