#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_types.hpp"
#include "enums.hpp"
#include "nm/dbus_nm_connection_settings.h"
#include "nm/dbus_nm_active_connection.h"

namespace qs::network {

class NMConnectionSettingsAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMConnectionSettingsAdapter(const QString& path, QObject* parent = nullptr);
	void updateSettings();
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] ConnectionSettingsMap settings() const { return this->bSettings; };

signals:
	void settingsChanged();
	void ready();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettingsAdapter, ConnectionSettingsMap, bSettings, &NMConnectionSettingsAdapter::settingsChanged);
	// clang-format on

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMConnectionSettingsAdapter, connectionSettingsProperties);
	DBusNMConnectionSettingsProxy* proxy = nullptr;
};

class NMActiveConnectionAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMActiveConnectionAdapter(const QString& path, QObject* parent = nullptr);
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QDBusObjectPath connection() const { return this->bConnection; };
	[[nodiscard]] bool isDefault() const { return this->bIsDefault; };
	[[nodiscard]] bool isDefault6() const { return this->bIsDefault6; };
	[[nodiscard]] NMActiveConnectionState::Enum state() const { return this->mState; };
	[[nodiscard]] NMActiveConnectionStateReason::Enum stateReason() const { return this->mStateReason; };

signals:
	void stateChanged(NMActiveConnectionState::Enum state, NMActiveConnectionStateReason::Enum reason);
	void connectionChanged(QDBusObjectPath path);
	void isDefaultChanged(bool isDefault);
	void isDefault6Changed(bool isDefault6);
	void ready();

private slots:
	void onStateChanged(quint32 state, quint32 reason);

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnectionAdapter, QDBusObjectPath, bConnection, &NMActiveConnectionAdapter::connectionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnectionAdapter, bool, bIsDefault, &NMActiveConnectionAdapter::isDefaultChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnectionAdapter, bool, bIsDefault6, &NMActiveConnectionAdapter::isDefault6Changed);
	// clang-format on

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMActiveConnectionAdapter, activeConnectionProperties);
	QS_DBUS_PROPERTY_BINDING(NMActiveConnectionAdapter, pConnection, bConnection, activeConnectionProperties, "Connection");
	QS_DBUS_PROPERTY_BINDING(NMActiveConnectionAdapter, pIsDefault, bIsDefault, activeConnectionProperties, "Default");
	QS_DBUS_PROPERTY_BINDING(NMActiveConnectionAdapter, pIsDefault6, bIsDefault6, activeConnectionProperties, "Default6");
	DBusNMActiveConnectionProxy* proxy = nullptr;

	NMActiveConnectionState::Enum mState = NMActiveConnectionState::Unknown;
	NMActiveConnectionStateReason::Enum mStateReason = NMActiveConnectionStateReason::Unknown;
};

} // namespace qs::network
