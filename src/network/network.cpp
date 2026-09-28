#include "network.hpp"
#include <utility>

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>

#include "../core/logcat.hpp"
#include "enums.hpp"
#include "known_network.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

Network::Network(QString name, NetworkDevice* device, QObject* parent)
    : QObject(parent)
    , bName(std::move(name))
    , mDevice(device) {
	this->bStateChanging.setBinding([this] {
		auto state = this->bState.value();
		return state == ConnectionState::Connecting || state == ConnectionState::Disconnecting;
	});
};

void Network::connect() {
	if (this->bConnected) {
		qCCritical(logNetwork) << this << "is already connected.";
		return;
	}
	this->requestConnect();
}

void Network::connectToKnownNetwork(KnownNetwork* knownNet) {
	if (this->bConnected) {
		qCCritical(logNetwork) << this << "is already connected.";
		return;
	}
	if (!this->bKnownNetworks.value().contains(knownNet)) return;
	this->requestConnectWithKnownNetwork(knownNet);
}

void Network::disconnect() {
	if (!this->bConnected) {
		qCCritical(logNetwork) << this << "is not currently connected";
		return;
	}
	this->requestDisconnect();
}

void Network::forget() { this->requestForget(); }

void Network::knownNetworkAdded(KnownNetwork* knownNet) {
	auto list = this->bKnownNetworks.value();
	if (list.contains(knownNet)) return;
	list.append(knownNet);
	this->bKnownNetworks = list;
}

void Network::knownNetworkRemoved(KnownNetwork* knownNet) {
	auto list = this->bKnownNetworks.value();
	if (!list.removeOne(knownNet)) return;
	this->bKnownNetworks = list;
}

} // namespace qs::network
