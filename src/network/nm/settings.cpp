#include "settings.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmetatype.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_nm_connection_settings.h"
#include "dbus_types.hpp"
#include "utils.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network", QtWarningMsg);
QS_LOGGING_CATEGORY(logNMSettings, "quickshell.network.nm_settings", QtWarningMsg);
} // namespace

NMSettings::NMSettings(const QString& path, QObject* parent): QObject(parent) {
	qDBusRegisterMetaType<QList<QVariantMap>>();
	qDBusRegisterMetaType<QList<quint32>>();
	qDBusRegisterMetaType<QList<QList<quint32>>>();
	qDBusRegisterMetaType<QList<QByteArray>>();
	qDBusRegisterMetaType<NMIPv6Address>();
	qDBusRegisterMetaType<QList<NMIPv6Address>>();
	qDBusRegisterMetaType<NMIPv6Route>();
	qDBusRegisterMetaType<QList<NMIPv6Route>>();

	this->proxy = new DBusNMConnectionSettingsProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection settings at"
		                             << path;
		return;
	}

	QObject::connect(
	    this->proxy,
	    &DBusNMConnectionSettingsProxy::Updated,
	    this,
	    &NMSettings::getSettings
	);

	this->bId.setBinding([this]() { return this->bSettings.value()["connection"]["id"].toString(); });
	this->bUuid.setBinding([this]() {
		return this->bSettings.value()["connection"]["uuid"].toString();
	});

	this->settingsProperties.setInterface(this->proxy);
	this->settingsProperties.updateAllViaGetAll();

	this->getSettings();
}

void NMSettings::getSettings() {
	auto pending = this->proxy->GetSettings();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<NMSettingsMap> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to get settings for" << this->path() << ":" << reply.error().message();
		} else {
			auto settings = reply.value();
			manualSettingDemarshall(settings);
			this->bSettings = settings;
			qCDebug(logNetworkManager) << "Settings map updated for" << this->path();

			if (!this->mLoaded) {
				emit this->loaded();
				this->mLoaded = true;
			}
		};

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

QDBusPendingCallWatcher* NMSettings::updateSettings(
    const NMSettingsMap& settingsToChange,
    const NMSettingsMap& settingsToRemove
) {
	auto settings = removeSettingsInMap(this->bSettings, settingsToRemove);
	settings = mergeSettingsMaps(settings, settingsToChange);
	auto pending = this->proxy->Update(settings);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	return call;
}

void NMSettings::clearSecrets() {
	auto pending = this->proxy->ClearSecrets();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to clear secrets for" << this->path() << ":" << reply.error().message();
		} else {
			qCDebug(logNetworkManager) << "Cleared secrets for" << this->path();
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
		} else {
			qCDebug(logNetworkManager) << "Successfully deletion of" << this->path();
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

QVariantMap NMSettings::read() {
	QVariantMap result;
	const auto& settings = this->bSettings.value();
	for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
		QVariantMap group;
		for (auto jt = it.value().constBegin(); jt != it.value().constEnd(); ++jt) {
			group.insert(jt.key(), settingTypeToQml(jt.value()));
		}
		result.insert(it.key(), group);
	}
	return result;
}

void NMSettings::write(const QVariantMap& settings) {
	NMSettingsMap changedSettings;
	NMSettingsMap removedSettings;
	QStringList failedSettings;

	for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
		if (!it.value().canConvert<QVariantMap>()) continue;

		auto group = it.value().toMap();
		QVariantMap toChange;
		QVariantMap toRemove;
		for (auto jt = group.constBegin(); jt != group.constEnd(); ++jt) {
			if (jt.value().isNull()) {
				toRemove.insert(jt.key(), QVariant());
			} else {
				auto converted = settingTypeFromQml(it.key(), jt.key(), jt.value());
				if (!converted.isValid()) failedSettings.append(it.key() + "." + jt.key());
				else toChange.insert(jt.key(), converted);
			}
		}
		if (!toChange.isEmpty()) changedSettings.insert(it.key(), toChange);
		if (!toRemove.isEmpty()) removedSettings.insert(it.key(), toRemove);
	}

	if (!failedSettings.isEmpty()) {
		qCWarning(logNMSettings) << "A write to" << this
		                         << "has received bad types for the following settings:"
		                         << failedSettings.join(", ");
	}

	auto* call = this->updateSettings(changedSettings, removedSettings);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to update settings for" << this->path() << ":" << reply.error().message();
		} else {
			qCDebug(logNMSettings) << "Successful write to" << this;
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

bool NMSettings::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMSettings::address() const { return this->proxy ? this->proxy->service() : QString(); }
QString NMSettings::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NMSettings* settings) {
	auto saver = QDebugStateSaver(debug);

	if (settings) {
		debug.nospace() << "NMSettings(" << static_cast<const void*>(settings)
		                << ", uuid=" << settings->uuid() << ")";
	} else {
		debug << "WifiNetwork(nullptr)";
	}

	return debug;
}
