#include "connection.hpp"

#include <qobject.h>
#include <qtmetamacros.h>

#include "nm/types.hpp"

namespace qs::network {

NMConnection::NMConnection(QObject* parent): QObject(parent) {}

void NMConnection::updateSettings(const ConnectionSettingsMap& settings) {
	emit this->requestUpdateSettings(settings);
}

void NMConnection::clearSecrets() { emit this->requestClearSecrets(); }

void NMConnection::forget() { emit this->requestForget(); }

} // namespace qs::network
