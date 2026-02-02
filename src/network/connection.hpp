#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtypes.h>

#include "nm/types.hpp"

namespace qs::network {

///! A NetworkManager connection settings profile.
class NMConnection: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// A settings map describing this network configuration.
	Q_PROPERTY(ConnectionSettingsMap settings READ default NOTIFY settingsChanged BINDABLE bindableSettings);
	/// A settings map describing the secrets belonging to this network configuration.
	Q_PROPERTY(ConnectionSettingsMap secretSettings READ default NOTIFY secretSettingsChanged BINDABLE bindableSecretSettings);
	/// A human readable unique identifier for the connection.
	Q_PROPERTY(QString id READ default NOTIFY idChanged BINDABLE bindableId);
	// clang-format on

public:
	explicit NMConnection(QObject* parent = nullptr);
	/// Attempt to update the connection with new settings and save the connection to disk. Secrets may be a part of the update request,
	/// and will either be stored in persistent storage tor sent to a Secret Agent for storage, depending on the flags
	/// associated with each secret and the presence of a registered Secret Agent.
	Q_INVOKABLE void updateSettings(const ConnectionSettingsMap& settings);
	/// Attempt to clear all of the secrets belonging to this connection.
	Q_INVOKABLE void clearSecrets();
	/// Delete the connection.
	Q_INVOKABLE void forget();

	QBindable<ConnectionSettingsMap> bindableSettings() { return &this->bSettings; }
	QBindable<ConnectionSettingsMap> bindableSecretSettings() { return &this->bSecretSettings; }
	QBindable<QString> bindableId() { return &this->bId; }

signals:
	void requestUpdateSettings(ConnectionSettingsMap settings);
	void requestClearSecrets();
	void requestForget();
	void settingsChanged();
	void secretSettingsChanged();
	void idChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnection, ConnectionSettingsMap, bSettings, &NMConnection::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnection, ConnectionSettingsMap, bSecretSettings, &NMConnection::secretSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnection, QString, bId, &NMConnection::idChanged);
	// clang-format on
};
} // namespace qs::network
