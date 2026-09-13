#include "wired.hpp"

#include <qobject.h>

#include "device.hpp"
#include "enums.hpp"
#include "network.hpp"

namespace qs::network {

WiredDevice::WiredDevice(QObject* parent): NetworkDevice(DeviceType::Wired, parent) {}

} // namespace qs::network
