#pragma once

#include <qdbusextratypes.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::bluetooth {

class BluetoothAgent: public QObject {
	Q_OBJECT;

public:
	explicit BluetoothAgent(QObject* parent = nullptr);

	[[nodiscard]] QString objectPath() const;

public Q_SLOTS:
	void RequestAuthorization(const QDBusObjectPath& device);
	void RequestConfirmation(const QDBusObjectPath& device, quint32 passkey);
	void Cancel();
	void Release();

private:
	QString mObjectPath;
};

} // namespace qs::bluetooth