#include "agent.hpp"

#include <qdbusextratypes.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstringliteral.h>

namespace qs::bluetooth {

BluetoothAgent::BluetoothAgent(QObject* parent)
    : QObject(parent)
    , mObjectPath(QStringLiteral("/org/quickshell/bluetooth/agent")) {}

QString BluetoothAgent::objectPath() const { return this->mObjectPath; }

void BluetoothAgent::RequestAuthorization(const QDBusObjectPath& device) { Q_UNUSED(device); }

void BluetoothAgent::RequestConfirmation(const QDBusObjectPath& device, quint32 passkey) {
	Q_UNUSED(device);
	Q_UNUSED(passkey);
}

void BluetoothAgent::Cancel() {}

void BluetoothAgent::Release() {}

} // namespace qs::bluetooth