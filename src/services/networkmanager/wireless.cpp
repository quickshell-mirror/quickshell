#include "wireless.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_wireless.h"

namespace qs::service::networkmanager {

NMWireless::NMWireless(QObject* parent): QObject(parent) {}

QString NMWirelessMode::toString(NMWirelessMode::Enum mode) {
	switch (mode) {
	case NMWirelessMode::Unknown: return "Unknown";
	case NMWirelessMode::Adhoc: return "Adhoc";
	case NMWirelessMode::Infra: return "Infra";
	case NMWirelessMode::AP: return "AP";
	case NMWirelessMode::Mesh: return "Mesh";
	default: return "Unknown";
	}
}

} // namespace qs::service::networkmanager

namespace qs::dbus {

using namespace qs::service::networkmanager;

DBusResult<NMWirelessMode::Enum> DBusDataTransform<NMWirelessMode::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<NMWirelessMode::Enum>(wire));
}

} // namespace qs::dbus
