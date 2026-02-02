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
#include "dbus_nm_connection_settings.h"
#include "dbus_types.hpp"

namespace qs::network {

// Proxy of a /org/freedesktop/NetworkManager/Settings/Connection/* object.
///! A NetworkManager connection settings profile.
class NMSettings: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	/// The human-readable unique identifier for the connection.
	Q_PROPERTY(QString id READ default NOTIFY idChanged BINDABLE bindableId);
	/// A universally unique identifier for the connection.
	Q_PROPERTY(QString uuid READ uuid NOTIFY uuidChanged BINDABLE bindableUuid);

public:
	explicit NMSettings(const QString& path, QObject* parent = nullptr);

	/// Clear all of the secrets belonging to the settings.
	Q_INVOKABLE void clearSecrets();
	/// Delete the settings.
	Q_INVOKABLE void forget();
	/// Update the connection with new settings and save the connection to disk.
	/// Only changed fields need to be included.
	/// Writing a setting to `null` will remove the setting or reset it to its default.
	///
	/// > [!NOTE] Secrets may be part of the update request,
	/// > and will be either stored in persistent storage or sent to a Secret Agent for storage,
	/// > depending on the flags associated with each secret.
	Q_INVOKABLE void write(const QVariantMap& settings);
	/// Get the settings map describing this network configuration.
	///
	/// > [!NOTE] This will never include any secrets required for connection to the network, as those are often protected.
	Q_INVOKABLE QVariantMap read();

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] NMSettingsMap map() { return this->bSettings; }
	QDBusPendingCallWatcher*
	updateSettings(const NMSettingsMap& settingsToChange, const NMSettingsMap& settingsToRemove = {});
	QBindable<QString> bindableId() { return &this->bId; }
	[[nodiscard]] QString uuid() const { return this->bUuid; }
	QBindable<QString> bindableUuid() { return &this->bUuid; }

signals:
	void loaded();
	void settingsChanged(NMSettingsMap settings);
	void idChanged(QString id);
	void uuidChanged(QString uuid);

private:
	bool mLoaded = false;

	void getSettings();

	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, NMSettingsMap, bSettings, &NMSettings::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, QString, bId, &NMSettings::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMSettings, QString, bUuid, &NMSettings::uuidChanged);
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMSettings, settingsProperties);
	DBusNMConnectionSettingsProxy* proxy = nullptr;
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NMSettings* settings);
