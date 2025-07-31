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

NMConnectionAdapter::NMConnectionAdapter(const QString& path, QObject* parent): QObject(parent) {
	qDBusRegisterMetaType<ConnectionSettingsMap>();

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

	// clang-format off
	QObject::connect(this->proxy, &DBusNMConnectionProxy::Updated, this, &NMConnectionAdapter::updateSettings);
	// clang-format on

	this->connectionProperties.setInterface(this->proxy);
	this->connectionProperties.updateAllViaGetAll();

	this->updateSettings();
}

void NMConnectionAdapter::updateSettings() {
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

bool NMConnectionAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMConnectionAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMConnectionAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
