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
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../known_network.hpp"
#include "dbus_nm_connection.h"
#include "dbus_types.hpp"
#include "utils.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMConnection::NMConnection(const QString& path, QObject* parent): QObject(parent) {
	qDBusRegisterMetaType<QList<QVariantMap>>();
	qDBusRegisterMetaType<QList<quint32>>();
	qDBusRegisterMetaType<QList<QList<quint32>>>();
	qDBusRegisterMetaType<QList<QByteArray>>();
	qDBusRegisterMetaType<NMIPv6Address>();
	qDBusRegisterMetaType<QList<NMIPv6Address>>();
	qDBusRegisterMetaType<NMIPv6Route>();
	qDBusRegisterMetaType<QList<NMIPv6Route>>();
	qDBusRegisterMetaType<QMap<QString, QString>>();

	this->proxy = new DBusNMConnectionProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection at" << path;
		return;
	}

	QObject::connect(this->proxy, &DBusNMConnectionProxy::Updated, this, &NMConnection::getSettings);

	QObject::connect(
	    this,
	    &NMConnection::settingsChanged,
	    this,
	    &NMConnection::loaded,
	    Qt::SingleShotConnection
	);

	this->connectionProperties.setInterface(this->proxy);
	this->connectionProperties.updateAllViaGetAll();

	this->mFrontend = new KnownNetwork(this);
	this->bindFrontend();

	this->getSettings();
}

void NMConnection::bindFrontend() {
	auto* frontend = this->mFrontend;
	frontend->bindableName().setBinding([this]() {
		return this->settings().value("connection").value("id").toString();
	});
	frontend->bindableSettings().setBinding([this]() { return settingsToQml(this->settings()); });

	QObject::connect(frontend, &KnownNetwork::requestForget, this, &NMConnection::forget);
	QObject::connect(frontend, &KnownNetwork::requestWrite, this, &NMConnection::write);
}

void NMConnection::getSettings() {
	auto pending = this->proxy->GetSettings();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<NMSettings> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to get settings for" << this->path() << ":" << reply.error().message();
		} else {
			auto settings = reply.value();
			manualSettingDemarshall(settings);
			this->bSettings = settings;
			qCDebug(logNetworkManager) << "Read settings for" << this->path();
		};

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

QDBusPendingCallWatcher*
NMConnection::update(const NMSettings& settingsToChange, const NMSettings& settingsToRemove) {
	auto settings = removeSettings(this->bSettings, settingsToRemove);
	settings = mergeSettings(settings, settingsToChange);
	auto pending = this->proxy->Update(settings);
	return new QDBusPendingCallWatcher(pending, this);
}

void NMConnection::forget() {
	auto pending = this->proxy->Delete();
	auto* call = new QDBusPendingCallWatcher(pending, this);
	QObject::connect(
	    call,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* call) {
		    const QDBusPendingReply<> reply = *call;
		    if (reply.isError()) {
			    qCWarning(logNetworkManager)
			        << "Failed to forget" << this->path() << ":" << reply.error().message();
		    } else {
			    qCDebug(logNetworkManager) << "Forgot" << this->path();
		    }
		    delete call;
	    }
	);
}

void NMConnection::write(const QVariantMap& settings) {
	NMSettings changedSettings;
	NMSettings removedSettings;
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
		qCWarning(logNetworkManager) << "A write to" << this
		                             << "has received bad types for the following settings:"
		                             << failedSettings.join(", ");
	}

	auto* call = this->update(changedSettings, removedSettings);
	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to update settings for" << this->path() << ":" << reply.error().message();
		} else {
			qCDebug(logNetworkManager) << "Updated settings for" << this->path();
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

bool NMConnection::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMConnection::address() const { return this->proxy ? this->proxy->service() : QString(); }
QString NMConnection::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
