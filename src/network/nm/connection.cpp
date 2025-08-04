#include "connection.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

// NMConnectionAdapter

NMConnectionSettingsAdapter::NMConnectionSettingsAdapter(const QString& path, QObject* parent)
    : QObject(parent) {
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
	QObject::connect(this->proxy, &DBusNMConnectionSettingsProxy::Updated, this, &NMConnectionSettingsAdapter::updateSettings);
	// clang-format on

	this->connectionSettingsProperties.setInterface(this->proxy);
	this->connectionSettingsProperties.updateAllViaGetAll();

	this->updateSettings();
}

void NMConnectionSettingsAdapter::updateSettings() {
	auto pending = this->proxy->GetSettings();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<ConnectionSettingsMap> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to get settings: " << reply.error().message();
		} else {
			this->bSettings = reply.value();
		}

		emit this->ready();
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

bool NMConnectionSettingsAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMConnectionSettingsAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMConnectionSettingsAdapter::path() const {
	return this->proxy ? this->proxy->path() : QString();
}

NMActiveConnectionAdapter::NMActiveConnectionAdapter(const QString& path, QObject* parent)
    : QObject(parent) {

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
	QObject::connect(&this->activeConnectionProperties, &DBusPropertyGroup::getAllFinished, this, [this]() { emit this->ready(); }, Qt::SingleShotConnection);
	QObject::connect(this->proxy, &DBusNMActiveConnectionProxy::StateChanged, this, &NMActiveConnectionAdapter::onStateChanged);
	// clang-format on

	this->activeConnectionProperties.setInterface(this->proxy);
	this->activeConnectionProperties.updateAllViaGetAll();
}

void NMActiveConnectionAdapter::onStateChanged(quint32 state, quint32 reason) {
	auto enumState = static_cast<NMActiveConnectionState::Enum>(state);
	auto enumReason = static_cast<NMActiveConnectionStateReason::Enum>(reason);

	if (enumState != mState) {
		this->mState = enumState;
		this->mStateReason = enumReason;
		emit this->stateChanged(enumState, enumReason);
	}
}

bool NMActiveConnectionAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMActiveConnectionAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMActiveConnectionAdapter::path() const {
	return this->proxy ? this->proxy->path() : QString();
}

} // namespace qs::network
