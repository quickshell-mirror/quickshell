#pragma once
#include <qobject.h>
#include <qdbusextratypes.h>
#include "dbus_service.h"

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
