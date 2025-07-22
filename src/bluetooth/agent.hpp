#pragma once

#include <qdbuscontext.h>
#include <qdbusextratypes.h>
#include <qdbusmessage.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

namespace qs::bluetooth {

class BluetoothPairingRequestType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Enum type");

public:
	enum Enum { Authorization, Confirmation, PinCode, Passkey, ServiceAuthorization };
	Q_ENUM(Enum);
};

class BluetoothAgent
    : public QObject
    , protected QDBusContext {
	Q_OBJECT;
	QML_ELEMENT;

public:
	explicit BluetoothAgent(QObject* parent = nullptr);

	[[nodiscard]] QString objectPath() const;

	Q_INVOKABLE void respondToRequest(quint32 token, bool accept);
	Q_INVOKABLE void respondWithPinCode(quint32 token, const QString& pinCode);
	Q_INVOKABLE void respondWithPasskey(quint32 token, quint32 passkey);

	// NOLINTBEGIN
	void RequestAuthorization(const QDBusObjectPath& device);
	void RequestConfirmation(const QDBusObjectPath& device, quint32 passkey);
	void AuthorizeService(const QDBusObjectPath& device, const QString& uuid);
	void RequestPinCode(const QDBusObjectPath& device);
	void RequestPasskey(const QDBusObjectPath& device);
	void DisplayPinCode(const QDBusObjectPath& device, const QString& pincode);
	void DisplayPasskey(const QDBusObjectPath& device, quint32 passkey, quint16 entered);
	void Cancel();
	void Release();
	// NOLINTEND

signals:
	void pairingRequested(
	    const QString& deviceAddress,
	    BluetoothPairingRequestType::Enum type,
	    quint32 passkey,
	    quint32 token
	);
	void agentReleased();

private:
	struct PendingRequest {
		QDBusMessage message;
		BluetoothPairingRequestType::Enum type;
		QString devicePath;
		quint32 serial;
	};

	[[nodiscard]] static QString getDeviceAddress(const QDBusObjectPath& device);
	void enqueuePendingRequest(const QDBusObjectPath& device, BluetoothPairingRequestType::Enum type);

	QString mObjectPath;
	QHash<quint32, PendingRequest> mPendingRequests;
	quint32 mLastSerial = 0;
};

} // namespace qs::bluetooth