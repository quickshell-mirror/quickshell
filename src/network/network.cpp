#include "network.hpp"
#include <utility>

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>

#include "../core/logcat.hpp"
#include "enums.hpp"
#include "nm/settings.hpp"

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

void Network::connectWithSettings(NMSettings* settings) {
	if (this->bConnected) {
		qCCritical(logNetwork) << this << "is already connected.";
		return;
	}
	if (this->bNmSettings.value().indexOf(settings) == -1) return;
	this->requestConnectWithSettings(settings);
}

void Network::disconnect() {
	if (!this->bConnected) {
		qCCritical(logNetwork) << this << "is not currently connected";
		return;
	}
	this->requestDisconnect();
}

void Network::forget() { this->requestForget(); }

void Network::settingsAdded(NMSettings* settings) {
	auto list = this->bNmSettings.value();
	if (list.contains(settings)) return;
	list.append(settings);
	this->bNmSettings = list;
}

void Network::settingsRemoved(NMSettings* settings) {
	auto list = this->bNmSettings.value();
	list.removeOne(settings);
	this->bNmSettings = list;
}

} // namespace qs::network
