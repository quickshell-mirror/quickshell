#pragma once
#include <qobject.h>
#include <qdbusextratypes.h>
#include "dbus_service.h"
#include <qtypes.h>

// Defining the "au" debus type
using NetworkManagerCapabilities = QList<quint32>;
Q_DECLARE_METATYPE(NetworkManagerCapabilities);

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
