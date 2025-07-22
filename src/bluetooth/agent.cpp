#include "agent.hpp"
#include <atomic>

#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qdbusextratypes.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstringliteral.h>

#include "../core/logcat.hpp"

namespace qs::bluetooth {

namespace {
QS_LOGGING_CATEGORY(logAgent, "quickshell.bluetooth.agent", QtWarningMsg);
std::atomic<quint32> gTokenCounter {1};
} // namespace

BluetoothAgent::BluetoothAgent(QObject* parent)
    : QObject(parent)
    , mObjectPath(QStringLiteral("/org/quickshell/bluetooth/agent")) {}

QString BluetoothAgent::objectPath() const { return this->mObjectPath; }

QString BluetoothAgent::getDeviceAddress(const QDBusObjectPath& device) {
	auto path = device.path();
	auto lastSlash = path.lastIndexOf('/');

	if (lastSlash >= 0) {
		auto devicePart = path.mid(lastSlash + 1);

		if (devicePart.startsWith("dev_")) {
			auto macPart = devicePart.mid(4);
			return macPart.replace('_', ':');
		}
	}

	return path;
}

void BluetoothAgent::enqueuePendingRequest(
    const QDBusObjectPath& device,
    BluetoothPairingRequestType::Enum type
) {
	auto msg = message();
	PendingRequest request;
	request.message = msg;
	request.type = type;
	request.devicePath = device.path();
	request.serial = gTokenCounter.fetch_add(1);

	this->mPendingRequests.insert(request.serial, request);
	this->mLastSerial = request.serial;
	setDelayedReply(true);
}

void BluetoothAgent::RequestAuthorization(const QDBusObjectPath& device) {
	qCDebug(logAgent) << "Authorization request for device" << device.path();
	this->enqueuePendingRequest(device, BluetoothPairingRequestType::Authorization);
	emit this->pairingRequested(
	    getDeviceAddress(device),
	    BluetoothPairingRequestType::Authorization,
	    0,
	    this->mLastSerial
	);
}

void BluetoothAgent::RequestConfirmation(const QDBusObjectPath& device, quint32 passkey) {
	qCDebug(logAgent) << "Confirmation request for device" << device.path() << "passkey" << passkey;
	this->enqueuePendingRequest(device, BluetoothPairingRequestType::Confirmation);
	emit this->pairingRequested(
	    getDeviceAddress(device),
	    BluetoothPairingRequestType::Confirmation,
	    passkey,
	    this->mLastSerial
	);
}

void BluetoothAgent::AuthorizeService(const QDBusObjectPath& device, const QString& uuid) {
	qCDebug(logAgent) << "Service authorization request for device" << device.path() << "uuid"
	                  << uuid;
	this->enqueuePendingRequest(device, BluetoothPairingRequestType::ServiceAuthorization);
	emit this->pairingRequested(
	    getDeviceAddress(device),
	    BluetoothPairingRequestType::ServiceAuthorization,
	    0,
	    this->mLastSerial
	);
}

void BluetoothAgent::RequestPinCode(const QDBusObjectPath& device) {
	qCDebug(logAgent) << "PIN code request for device" << device.path();
	this->enqueuePendingRequest(device, BluetoothPairingRequestType::PinCode);
	emit this->pairingRequested(
	    getDeviceAddress(device),
	    BluetoothPairingRequestType::PinCode,
	    0,
	    this->mLastSerial
	);
}

void BluetoothAgent::RequestPasskey(const QDBusObjectPath& device) {
	qCDebug(logAgent) << "Passkey request for device" << device.path();
	this->enqueuePendingRequest(device, BluetoothPairingRequestType::Passkey);
	emit this->pairingRequested(
	    getDeviceAddress(device),
	    BluetoothPairingRequestType::Passkey,
	    0,
	    this->mLastSerial
	);
}

void BluetoothAgent::DisplayPinCode(const QDBusObjectPath& device, const QString& pincode) {
	qCDebug(logAgent) << "Display PIN code for device" << device.path() << "pincode" << pincode;
	emit this->pairingRequested(getDeviceAddress(device), BluetoothPairingRequestType::PinCode, 0, 0);
}

void BluetoothAgent::DisplayPasskey(
    const QDBusObjectPath& device,
    quint32 passkey,
    quint16 entered
) {
	qCDebug(logAgent) << "Display passkey for device" << device.path() << "passkey" << passkey
	                  << "entered" << entered;
	emit this->pairingRequested(
	    getDeviceAddress(device),
	    BluetoothPairingRequestType::Passkey,
	    passkey,
	    0
	);
}

void BluetoothAgent::Cancel() {
	qCDebug(logAgent) << "Agent operation cancelled";

	if (this->mLastSerial != 0 && this->mPendingRequests.contains(this->mLastSerial)) {
		qCDebug(logAgent) << "Cancelling most recent request serial" << this->mLastSerial;
		this->mPendingRequests.remove(this->mLastSerial);
	}
}

void BluetoothAgent::Release() {
	qCDebug(logAgent) << "Agent released by BlueZ";

	for (auto& request: this->mPendingRequests) {
		auto reply = request.message.createErrorReply(QDBusError::NoReply, "Agent released");
		QDBusConnection::systemBus().send(reply);
	}

	this->mPendingRequests.clear();
	this->mLastSerial = 0;

	emit this->agentReleased();
}

void BluetoothAgent::respondToRequest(quint32 token, bool accept) {
	auto it = this->mPendingRequests.find(token);
	if (it == this->mPendingRequests.end()) return;

	auto request = it.value();
	this->mPendingRequests.erase(it);

	qCDebug(logAgent) << (accept ? "Accepting" : "Rejecting") << "request token" << token;

	auto reply = accept ? request.message.createReply()
	                    : request.message.createErrorReply(QDBusError::AccessDenied, "User rejected");

	QDBusConnection::systemBus().send(reply);
}

void BluetoothAgent::respondWithPinCode(quint32 token, const QString& pinCode) {
	auto it = this->mPendingRequests.find(token);
	if (it == this->mPendingRequests.end() || it->type != BluetoothPairingRequestType::PinCode)
		return;

	auto request = it.value();
	this->mPendingRequests.erase(it);

	qCDebug(logAgent) << "Responding with PIN code:" << pinCode << "for token" << token;

	auto reply = request.message.createReply();
	reply << pinCode;

	QDBusConnection::systemBus().send(reply);
}

void BluetoothAgent::respondWithPasskey(quint32 token, quint32 passkey) {
	auto it = this->mPendingRequests.find(token);
	if (it == this->mPendingRequests.end() || it->type != BluetoothPairingRequestType::Passkey)
		return;

	auto request = it.value();
	this->mPendingRequests.erase(it);

	qCDebug(logAgent) << "Responding with passkey:" << passkey << "for token" << token;

	auto reply = request.message.createReply();
	reply << passkey;

	QDBusConnection::systemBus().send(reply);
}

} // namespace qs::bluetooth