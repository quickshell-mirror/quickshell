#pragma once

#include <qcontainerfwd.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../../core/logcat.hpp"
#include "dbus_watcher_interface.h"
#include "item.hpp"

QS_DECLARE_LOGGING_CATEGORY(logStatusNotifierHost);

namespace qs::service::sni {

class StatusNotifierHost: public QObject {
	Q_OBJECT;

public:
	explicit StatusNotifierHost(QObject* parent = nullptr);

	void connectToWatcher();
	[[nodiscard]] QList<StatusNotifierItem*> items() const;
	[[nodiscard]] StatusNotifierItem* itemByService(const QString& service) const;

	static StatusNotifierHost* instance();

signals:
	void itemRegistered(qs::service::sni::StatusNotifierItem* item);
	void itemReady(qs::service::sni::StatusNotifierItem* item);
	void itemUnregistered(qs::service::sni::StatusNotifierItem* item);

private slots:
	void onWatcherRegistered();
	void onWatcherUnregistered();
	void onItemRegistered(const QString& item);
	void onItemUnregistered(const QString& item);
	void onItemReady();

private:
	QString hostId;
	QDBusServiceWatcher serviceWatcher;
	DBusStatusNotifierWatcher* watcher = nullptr;
	QHash<QString, StatusNotifierItem*> mItems;
};

} // namespace qs::service::sni
