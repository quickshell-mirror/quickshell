#include "wired.hpp"

#include <qobject.h>

#include "device.hpp"
#include "enums.hpp"
#include "network.hpp"

namespace qs::network {

WiredDevice::WiredDevice(QObject* parent): NetworkDevice(DeviceType::Wired, parent) {}

void WiredDevice::networkAdded(Network* net) {
	this->NetworkDevice::networkAdded(net);
	this->bNetwork = net;
};
void WiredDevice::networkRemoved(Network* net) {
	this->NetworkDevice::networkRemoved(net);
	this->bNetwork = nullptr;
};

} // namespace qs::network
