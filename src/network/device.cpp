#include "device.hpp"

#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
} // namespace

NetworkDevice::NetworkDevice(QObject* parent): QObject(parent) {};

void NetworkDevice::setName(const QString& name) {
	if (name == this->mName) return;
	this->mName = name;
	emit this->nameChanged();
}

void NetworkDevice::setAddress(const QString& address) {
	if (address == this->mAddress) return;
	this->mAddress = address;
	emit this->addressChanged();
}

void NetworkDevice::setState(NetworkConnectionState::Enum state) {
	if (state == this->mState) return;
	this->mState = state;
	emit this->stateChanged();
}

void NetworkDevice::setNmState(NMDeviceState::Enum nmState) {
	if (nmState == this->mNmState) return;
	this->mNmState = nmState;
	emit this->nmStateChanged();
}

void NetworkDevice::disconnect() {
	if (this->mState == NetworkConnectionState::Disconnected) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnected";
		return;
	}
	if (this->mState == NetworkConnectionState::Disconnecting) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnecting";
		return;
	}
	qCDebug(logNetworkDevice) << "Disconnecting from device" << this;
	this->requestDisconnect();
}

QString NetworkConnectionState::toString(NetworkConnectionState::Enum state) {
	switch (state) {
	case Unknown: return "Unknown";
	case Connecting: return "Connecting";
	case Connected: return "Connected";
	case Disconnecting: return "Disconnecting";
	case Disconnected: return "Disconnected";
	}
	return {};
}

} // namespace qs::network
