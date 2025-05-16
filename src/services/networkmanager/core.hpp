#pragma once
#include <qobject.h>
#include <qdbusextratypes.h>
#include "dbus_service.h"
#include <qtypes.h>

// Define the "au" dbus type
using dbus_type_au = QList<quint32>;
Q_DECLARE_METATYPE(dbus_type_au);
// Define the "a{sv}" dbus type
//using NetworkManagerCapabilities = QList<quint32>;

namespace qs::service::networkmanager {

class NetworkManager : public QObject {	
	Q_OBJECT;
public:
	static NetworkManager* instance(); // Get instance of the class
private:
	DBUSNetworkservice* service = nullptr;
	void init();
};

}
