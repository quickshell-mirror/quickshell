#pragma once

#include <functional>

#include <qcontainerfwd.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/model.hpp"
#include "notification.hpp"

namespace qs::service::notifications {

struct NotificationServerSupport {
	bool persistence = false;
	bool body = true;
	bool bodyMarkup = false;
	bool bodyHyperlinks = false;
	bool bodyImages = false;
	bool actions = false;
	bool actionIcons = false;
	bool image = false;
	bool inlineReply = false;
	QVector<QString> extraHints;
};

class NotificationServer: public QObject {
	Q_OBJECT;

public:
	static NotificationServer* instance();

	void switchGeneration(bool reEmit, const std::function<void()>& clearHook);
	ObjectModel<Notification>* trackedNotifications();
	void deleteNotification(Notification* notification, NotificationCloseReason::Enum reason);

	// NOLINTBEGIN
	void CloseNotification(uint id);
	QStringList GetCapabilities() const;
	static QString GetServerInformation(QString& vendor, QString& version, QString& specVersion);
	uint Notify(
	    const QString& appName,
	    uint replacesId,
	    const QString& appIcon,
	    const QString& summary,
	    const QString& body,
	    const QStringList& actions,
	    const QVariantMap& hints,
	    int expireTimeout
	);
	// NOLINTEND

	NotificationServerSupport support;

signals:
	void notification(Notification* notification);

	// NOLINTBEGIN
	void NotificationClosed(quint32 id, quint32 reason);
	void ActionInvoked(quint32 id, QString action);
	void NotificationReplied(quint32 id, QString replyText);
	// NOLINTEND

private slots:
	static void onServiceUnregistered(const QString& service);

private:
	explicit NotificationServer();

	static void tryRegister();

	QDBusServiceWatcher serviceWatcher;
	quint32 nextId = 1;
	QHash<quint32, Notification*> idMap;
	ObjectModel<Notification> mNotifications {this};
};

} // namespace qs::service::notifications
