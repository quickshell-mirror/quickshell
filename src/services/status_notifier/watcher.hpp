#pragma once

#include <qdbuscontext.h>
#include <qdbusinterface.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"

QS_DECLARE_LOGGING_CATEGORY(logStatusNotifierWatcher);

namespace qs::service::sni {

class StatusNotifierWatcher
    : public QObject
    , protected QDBusContext {
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
	[[nodiscard]] bool isRegistered() const;

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
	QString qualifiedItem(const QString& item);

	QDBusServiceWatcher serviceWatcher;
	QList<QString> hosts;
	QList<QString> items;
	bool registered = false;
};

} // namespace qs::service::sni
