#include "wifi.hpp"

#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logWifiDevice, "quickshell.wifi.device", QtWarningMsg);
Q_LOGGING_CATEGORY(logWifiNetwork, "quickshell.wifi.network", QtWarningMsg);
Q_LOGGING_CATEGORY(logWifi, "quickshell.wifi", QtWarningMsg);
} // namespace

WifiDevice::WifiDevice(QObject* parent): NetworkDevice(parent) {};

void WifiDevice::scanComplete() {
	if (this->mScanning) {
		this->mScanning = false;
		emit this->scanningChanged();
	}
}

void WifiDevice::scan() {
	if (this->mScanning) {
		qCCritical(logWifiDevice) << "Wireless device" << this->name() << "is already scanning";
		return;
	}

	qCDebug(logWifiDevice) << "Requesting scan on wireless device" << this->name();
	this->mScanning = true;
	emit this->scanningChanged();
	this->requestScan();
}

void WifiDevice::networkAdded(WifiNetwork* net) { this->mNetworks.insertObject(net); }
void WifiDevice::networkRemoved(WifiNetwork* net) { this->mNetworks.removeObject(net); }

WifiNetwork::WifiNetwork(QString ssid, QObject* parent): QObject(parent), mSsid(std::move(ssid)) {};

void WifiNetwork::setSignalStrength(quint8 signal) {
	if (this->mSignalStrength == signal) return;
	this->mSignalStrength = signal;
	emit this->signalStrengthChanged();
}

void WifiNetwork::setState(NetworkConnectionState::Enum state) {
	if (this->mState == state) return;
	this->mState = state;
	emit this->stateChanged();
}

void WifiNetwork::setKnown(bool known) {
	if (this->mKnown == known) return;
	this->mKnown = known;
	emit this->knownChanged();
}

void WifiNetwork::connect() {
	if (this->mState == NetworkConnectionState::Connected) {
		qCCritical(logWifiNetwork) << this->ssid() << "is already connected.";
		return;
	}
	if (this->mState == NetworkConnectionState::Connecting) {
		qCCritical(logWifiNetwork) << this->ssid() << "is already connecting.";
		return;
	}
	this->requestConnect();
}

void WifiNetwork::setNmReason(NMConnectionStateReason::Enum reason) {
	if (this->mNmReason == reason) return;
	this->mNmReason = reason;
	emit this->nmReasonChanged();
}

void WifiNetwork::setNmSecurity(NMWirelessSecurityType::Enum security) {
	if (this->mNmSecurity == security) return;
	this->mNmSecurity = security;
	emit this->nmSecurityChanged();
}

Wifi::Wifi(QObject* parent): QObject(parent) {};

void Wifi::onDeviceAdded(WifiDevice* dev) {
	this->mDevices.insertObject(dev);
	if (this->mDefaultDevice) return;
	this->setDefaultDevice(dev);
}

void Wifi::onDeviceRemoved(WifiDevice* dev) { this->mDevices.removeObject(dev); };

void Wifi::onHardwareEnabledSet(bool enabled) {
	if (this->mHardwareEnabled == enabled) return;
	this->mHardwareEnabled = enabled;
	emit this->hardwareEnabledChanged();
}

void Wifi::setEnabled(bool enabled) {
	if (this->mEnabled == enabled) {
		QString state = enabled ? "enabled" : "disabled";
		qCCritical(logWifi) << "Wifi is already" << state;
	} else {
		emit this->requestSetEnabled(enabled);
	}
}

void Wifi::onEnabledSet(bool enabled) {
	if (this->mEnabled == enabled) return;
	this->mEnabled = enabled;
	emit this->enabledChanged();
}

void Wifi::setDefaultDevice(WifiDevice* dev) {
	if (this->mDefaultDevice == dev) return;
	this->mDefaultDevice = dev;
	emit this->defaultDeviceChanged();
}

} // namespace qs::network
