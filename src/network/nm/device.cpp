#include "device.hpp"

#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qset.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "active_connection.hpp"
#include "dbus_nm_device.h"
#include "settings.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMDevice::NMDevice(const QString& path, QObject* parent): QObject(parent) {
	this->deviceProxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->deviceProxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for device at" << path;
		return;
	}

	// clang-format off
	QObject::connect(this, &NMDevice::availableSettingsPathsChanged, this, &NMDevice::onAvailableSettingsPathsChanged);
	QObject::connect(this, &NMDevice::activeConnectionPathChanged, this, &NMDevice::onActiveConnectionPathChanged);
	QObject::connect(this->deviceProxy, &DBusNMDeviceProxy::StateChanged, this, &NMDevice::onStateChanged);
	// clang-format on

	this->deviceProperties.setInterface(this->deviceProxy);
	this->deviceProperties.updateAllViaGetAll();
}

void NMDevice::onStateChanged(quint32 newState, quint32 /*oldState*/, quint32 reason) {
	auto enumReason = static_cast<NMDeviceStateReason::Enum>(reason);
	auto enumNewState = static_cast<NMDeviceState::Enum>(newState);
	if (enumNewState == NMDeviceState::Failed) this->bLastFailReason = enumReason;
	if (this->bStateReason == enumReason) return;
	this->bStateReason = enumReason;
}

void NMDevice::onActiveConnectionPathChanged(const QDBusObjectPath& path) {
	const QString stringPath = path.path();

	// Remove old active connection
	if (this->mActiveConnection) {
		QObject::disconnect(this->mActiveConnection, nullptr, this, nullptr);
		delete this->mActiveConnection;
		this->mActiveConnection = nullptr;
	}

	// Create new active connection
	if (stringPath != "/") {
		auto* active = new NMActiveConnection(stringPath, this);
		if (!active->isValid()) {
			qCWarning(logNetworkManager) << "Ignoring invalid registration of" << stringPath;
			delete active;
		} else {
			this->mActiveConnection = active;
			QObject::connect(
			    active,
			    &NMActiveConnection::loaded,
			    this,
			    [this, active]() { emit this->activeConnectionLoaded(active); },
			    Qt::SingleShotConnection
			);
		}
	}
}

void NMDevice::onAvailableSettingsPathsChanged(const QList<QDBusObjectPath>& paths) {
	QSet<QString> newPathSet;
	for (const QDBusObjectPath& path: paths) {
		newPathSet.insert(path.path());
	}
	const auto existingPaths = this->mSettings.keys();
	const QSet<QString> existingPathSet(existingPaths.begin(), existingPaths.end());

	const auto addedSettings = newPathSet - existingPathSet;
	const auto removedSettings = existingPathSet - newPathSet;

	for (const QString& path: addedSettings) {
		this->registerSettings(path);
	}
	for (const QString& path: removedSettings) {
		auto* connection = this->mSettings.take(path);
		if (!connection) {
			qCDebug(logNetworkManager) << "Sent removal signal for" << path << "which is not registered.";
		} else {
			delete connection;
		}
	};
}

void NMDevice::registerSettings(const QString& path) {
	auto* settings = new NMSettings(path, this);
	if (!settings->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete settings;
	} else {
		this->mSettings.insert(path, settings);
		QObject::connect(
		    settings,
		    &NMSettings::loaded,
		    this,
		    [this, settings]() { emit this->settingsLoaded(settings); },
		    Qt::SingleShotConnection
		);
	}
}

void NMDevice::disconnect() { this->deviceProxy->Disconnect(); }

void NMDevice::setAutoconnect(bool autoconnect) {
	if (autoconnect == this->bAutoconnect) return;
	this->bAutoconnect = autoconnect;
	this->pAutoconnect.write();
}

void NMDevice::setManaged(bool managed) {
	if (managed == this->bManaged) return;
	this->bManaged = managed;
	this->pManaged.write();
}

bool NMDevice::isValid() const { return this->deviceProxy && this->deviceProxy->isValid(); }
QString NMDevice::address() const {
	return this->deviceProxy ? this->deviceProxy->service() : QString();
}
QString NMDevice::path() const { return this->deviceProxy ? this->deviceProxy->path() : QString(); }

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMDeviceState::Enum>
DBusDataTransform<qs::network::NMDeviceState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceState::Enum>(wire));
}

} // namespace qs::dbus
