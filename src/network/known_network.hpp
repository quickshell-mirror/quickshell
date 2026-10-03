#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../core/doc.hpp"

namespace qs::network {

///! A known network profile.
///
/// Use @@read() to get its settings and @@write() to update them.
/// These functions return/accept a JavaScript object whose structure depends on the current @@Networking.backend.
///
///
/// #### NetworkManager Settings Object Schema
/// For NetworkManager, settings are grouped by category. For example:
/// ```qml
/// knownNetwork.write({
///   connection: { autoconnect: false },
///   ipv4: { method: "auto" }
/// });
/// ```
///
/// Here, `connection` and `ipv4` are categories; `autoconnect` and `method` are settings within them.
/// See the [category list](https://networkmanager.dev/docs/api/latest/ch01.html) and [settings reference](https://networkmanager.dev/docs/api/latest/nm-settings-dbus.html) for available names, types, and defaults.
/// The type of each setting value is the JavaScript equivalent of the documented DBus type.
/// An exception is byte array (ay) settings which are read and written as UTF-8 strings.
class KnownNetwork: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("KnownNetwork can only be acquired through Networking singleton");

	/// The human-readable name of the known network profile.
	///
	/// > [!NOTE] For NetworkManager, @@name is equivalent to the connection.id setting.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);

public:
	explicit KnownNetwork(QObject* parent = nullptr);

	/// Forget the known network profile.
	Q_INVOKABLE void forget();
	/// Update the settings of the known network profile.
	/// Only changed fields need to be included.
	/// Writing a setting to `null` will remove the setting or reset it to its default.
	Q_INVOKABLE void write(const QVariantMap& settings);
	/// Get the settings for this known network.
	Q_INVOKABLE QVariantMap read();

	QBindable<QString> bindableName() { return &this->bName; }
	QBindable<QVariantMap> bindableSettings() { return &this->bSettings; }

signals:
	QSDOC_HIDE void requestForget();
	QSDOC_HIDE void requestWrite(const QVariantMap& settings);
	QSDOC_HIDE void requestRead(QVariantMap* settings);

	void nameChanged();
	void settingsChanged();

protected:
	Q_OBJECT_BINDABLE_PROPERTY(KnownNetwork, QString, bName, &KnownNetwork::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(KnownNetwork, QVariantMap, bSettings, &KnownNetwork::settingsChanged);
};

} // namespace qs::network
