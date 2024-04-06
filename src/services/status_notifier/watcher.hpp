#pragma once

#include <qdbusinterface.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

Q_DECLARE_LOGGING_CATEGORY(logStatusNotifierWatcher);

namespace qs::service::sni {

class StatusNotifierWatcher: public QObject {
	Q_OBJECT;
	Q_PROPERTY(qint32 ProtocolVersion READ protocolVersion);
	Q_PROPERTY(bool IsStatusNotifierHostRegistered READ isHostRegistered);
	Q_PROPERTY(QList<QString> RegisteredStatusNotifierItems READ registeredItems);

public:
	explicit StatusNotifierWatcher(QObject* parent = nullptr);

	void tryRegister();

	[[nodiscard]] qint32 protocolVersion() const { return 0; } // NOLINT
	[[nodiscard]] bool isHostRegistered() const;
	[[nodiscard]] QList<QString> registeredItems() const;

	// NOLINTBEGIN
	void RegisterStatusNotifierHost(const QString& host);
	void RegisterStatusNotifierItem(const QString& item);
	// NOLINTEND

	static StatusNotifierWatcher* instance();

signals:
	// NOLINTBEGIN
	void StatusNotifierHostRegistered();
	void StatusNotifierHostUnregistered();
	void StatusNotifierItemRegistered(const QString& service);
	void StatusNotifierItemUnregistered(const QString& service);
	// NOLINTEND

private slots:
	void onServiceUnregistered(const QString& service);

private:
	QDBusServiceWatcher serviceWatcher;
	QList<QString> hosts;
	QList<QString> items;
};

} // namespace qs::service::sni
