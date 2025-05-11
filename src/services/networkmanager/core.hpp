#pragma once
#include <qobject.h>

namespace qs::service::networkmanager {

class NetworkManager : public QObject {	
	Q_OBJECT;
public:
	static NetworkManager* instance(); // Get instance of the class
	void test();
};

}
