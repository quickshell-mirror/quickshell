#include "connection.hpp"

#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"

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

	QObject::connect(
	    this->proxy,
	    &DBusNMConnectionSettingsProxy::Updated,
	    this,
	    &NMConnectionSettings::updateSettings
	);

	this->connectionSettingsProperties.setInterface(this->proxy);
	this->connectionSettingsProperties.updateAllViaGetAll();

	this->updateSettings();
}

void NMConnectionSettings::updateSettings() {
	auto pending = this->proxy->GetSettings();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<ConnectionSettingsMap> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to get" << this->path() << "settings:" << reply.error().message();
		} else {
			this->bSettings = reply.value();
		}

		emit this->ready();
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
	QObject::connect(&this->activeConnectionProperties, &DBusPropertyGroup::getAllFinished, this, &NMActiveConnection::ready, Qt::SingleShotConnection);
	QObject::connect(this->proxy, &DBusNMActiveConnectionProxy::StateChanged, this, &NMActiveConnection::onStateChanged);
	// clang-format on

	this->activeConnectionProperties.setInterface(this->proxy);
	this->activeConnectionProperties.updateAllViaGetAll();
}

void NMActiveConnection::onStateChanged(quint32 /*state*/, quint32 reason) {
	auto enumReason = static_cast<NMConnectionStateReason::Enum>(reason);
	if (this->mStateReason == enumReason) return;
	this->mStateReason = enumReason;
	emit this->stateReasonChanged(enumReason);
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
