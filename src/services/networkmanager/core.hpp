#pragma once
#include <qobject.h>
#include <qtmetamacros.h>

#include "dbus_service.h"

namespace qs::service::networkmanager {

class NetworkManager: public QObject {
	Q_OBJECT;

public:
	static NetworkManager* instance();

private:
	explicit NetworkManager();
	void init();
	DBusNetworkManagerService* service = nullptr;
};

} // namespace qs::service::networkmanager
