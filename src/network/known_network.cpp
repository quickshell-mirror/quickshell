#include "known_network.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::network {

KnownNetwork::KnownNetwork(QObject* parent): QObject(parent) {}

void KnownNetwork::forget() { emit this->requestForget(); }

void KnownNetwork::write(const QVariantMap& settings) { emit this->requestWrite(settings); }

QVariantMap KnownNetwork::read() { return this->bSettings; }

} // namespace qs::network
