#pragma once

#include <qbytearray.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qstringlist.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "dbus_nm_active_connection.h"
#include "enums.hpp"

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NMConnectionState::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMConnectionState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

// Proxy of a /org/freedesktop/NetworkManager/ActiveConnection/* object.
class NMActiveConnection: public QObject {
	Q_OBJECT;

public:
	explicit NMActiveConnection(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QDBusObjectPath connection() const { return this->bConnection; };
	[[nodiscard]] NMConnectionState::Enum state() const { return this->bState; };
	[[nodiscard]] NMNetworkStateReason::Enum stateReason() const { return this->bStateReason; };

signals:
	void loaded();
	void connectionChanged(QDBusObjectPath path);
	void stateChanged(NMConnectionState::Enum state);
	void stateReasonChanged(NMNetworkStateReason::Enum reason);

private slots:
	void onStateChanged(quint32 state, quint32 reason);

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnection, QDBusObjectPath, bConnection, &NMActiveConnection::connectionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnection, NMConnectionState::Enum, bState, &NMActiveConnection::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnection, NMNetworkStateReason::Enum, bStateReason, &NMActiveConnection::stateReasonChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMActiveConnection, activeConnectionProperties);
	QS_DBUS_PROPERTY_BINDING(NMActiveConnection, pConnection, bConnection, activeConnectionProperties, "Connection");
	QS_DBUS_PROPERTY_BINDING(NMActiveConnection, pState, bState, activeConnectionProperties, "State");
	// clang-format on
	DBusNMActiveConnectionProxy* proxy = nullptr;
};

} // namespace qs::network
