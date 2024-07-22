#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../../core/retainable.hpp"

namespace qs::service::notifications {

class NotificationImage;

///! The urgency level of a Notification.
/// See @@Notification.urgency.
class NotificationUrgency: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		Low = 0,
		Normal = 1,
		Critical = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(NotificationUrgency::Enum value);
};

///! The reason a Notification was closed.
/// See @@Notification.closed(s).
class NotificationCloseReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		/// The notification expired due to a timeout.
		Expired = 1,
		/// The notification was explicitly dismissed by the user.
		Dismissed = 2,
		/// The remote application requested the notification be removed.
		CloseRequested = 3,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(NotificationCloseReason::Enum value);
};

class NotificationAction;

///! A notification emitted by a NotificationServer.
/// A notification emitted by a NotificationServer.
///
/// > [!INFO] This type is @@Quickshell.Retainable. It
/// > can be retained after destruction if necessary.
class Notification
    : public QObject
    , public Retainable {
	Q_OBJECT;
	/// Id of the notification as given to the client.
	Q_PROPERTY(quint32 id READ id CONSTANT);
	/// If the notification is tracked by the notification server.
	///
	/// Setting this property to false is equivalent to calling @@dismiss().
	Q_PROPERTY(bool tracked READ isTracked WRITE setTracked NOTIFY trackedChanged);
	/// If this notification was carried over from the last generation
	/// when quickshell reloaded.
	///
	/// Notifications from the last generation will only be emitted
	/// if @@NotificationServer.keepOnReload is true.
	Q_PROPERTY(bool lastGeneration READ isLastGeneration CONSTANT);
	/// Time in seconds the notification should be valid for
	Q_PROPERTY(qreal expireTimeout READ expireTimeout NOTIFY expireTimeoutChanged);
	/// The sending application's name.
	Q_PROPERTY(QString appName READ appName NOTIFY appNameChanged);
	/// The sending application's icon. If none was provided, then the icon from an associated
	/// desktop entry will be retrieved. If none was found then "".
	Q_PROPERTY(QString appIcon READ appIcon NOTIFY appIconChanged);
	/// The image associated with this notification, or "" if none.
	Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged);
	Q_PROPERTY(QString body READ body NOTIFY bodyChanged);
	Q_PROPERTY(NotificationUrgency::Enum urgency READ urgency NOTIFY urgencyChanged);
	/// Actions that can be taken for this notification.
	Q_PROPERTY(QVector<NotificationAction*> actions READ actions NOTIFY actionsChanged);
	/// If actions associated with this notification have icons available.
	///
	/// See @@NotificationAction.identifier for details.
	Q_PROPERTY(bool hasActionIcons READ hasActionIcons NOTIFY hasActionIconsChanged);
	/// If true, the notification will not be destroyed after an action is invoked.
	Q_PROPERTY(bool resident READ isResident NOTIFY isResidentChanged);
	/// If true, the notification should skip any kind of persistence function like a notification area.
	Q_PROPERTY(bool transient READ isTransient NOTIFY isTransientChanged);
	/// The name of the sender's desktop entry or "" if none was supplied.
	Q_PROPERTY(QString desktopEntry READ desktopEntry NOTIFY desktopEntryChanged);
	/// An image associated with the notification.
	///
	/// This image is often something like a profile picture in instant messaging applications.
	Q_PROPERTY(QString image READ image NOTIFY imageChanged);
	/// All hints sent by the client application as a javascript object.
	/// Many common hints are exposed via other properties.
	Q_PROPERTY(QVariantMap hints READ hints NOTIFY hintsChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("Notifications must be acquired from a NotificationServer");

public:
	explicit Notification(quint32 id, QObject* parent): QObject(parent), mId(id) {}

	/// Destroy the notification and hint to the remote application that it has
	/// timed out an expired.
	Q_INVOKABLE void expire();
	/// Destroy the notification and hint to the remote application that it was
	/// explicitly closed by the user.
	Q_INVOKABLE void dismiss();

	void updateProperties(
	    const QString& appName,
	    QString appIcon,
	    const QString& summary,
	    const QString& body,
	    const QStringList& actions,
	    QVariantMap hints,
	    qint32 expireTimeout
	);

	void close(NotificationCloseReason::Enum reason);

	[[nodiscard]] quint32 id() const;

	[[nodiscard]] bool isTracked() const;
	[[nodiscard]] NotificationCloseReason::Enum closeReason() const;
	void setTracked(bool tracked);

	[[nodiscard]] bool isLastGeneration() const;
	void setLastGeneration();

	[[nodiscard]] qreal expireTimeout() const;
	[[nodiscard]] QString appName() const;
	[[nodiscard]] QString appIcon() const;
	[[nodiscard]] QString summary() const;
	[[nodiscard]] QString body() const;
	[[nodiscard]] NotificationUrgency::Enum urgency() const;
	[[nodiscard]] QVector<NotificationAction*> actions() const;
	[[nodiscard]] bool hasActionIcons() const;
	[[nodiscard]] bool isResident() const;
	[[nodiscard]] bool isTransient() const;
	[[nodiscard]] QString desktopEntry() const;
	[[nodiscard]] QString image() const;
	[[nodiscard]] QVariantMap hints() const;

signals:
	/// Sent when a notification has been closed.
	///
	/// The notification object will be destroyed as soon as all signal handlers exit.
	void closed(NotificationCloseReason::Enum reason);

	void trackedChanged();
	void expireTimeoutChanged();
	void appNameChanged();
	void appIconChanged();
	void summaryChanged();
	void bodyChanged();
	void urgencyChanged();
	void actionsChanged();
	void hasActionIconsChanged();
	void isResidentChanged();
	void isTransientChanged();
	void desktopEntryChanged();
	void imageChanged();
	void hintsChanged();

private:
	quint32 mId;
	NotificationCloseReason::Enum mCloseReason = NotificationCloseReason::Dismissed;
	bool mLastGeneration = false;
	qreal mExpireTimeout = 0;
	QString mAppName;
	QString mAppIcon;
	QString mSummary;
	QString mBody;
	NotificationUrgency::Enum mUrgency = NotificationUrgency::Normal;
	QVector<NotificationAction*> mActions;
	bool mHasActionIcons = false;
	bool mIsResident = false;
	bool mIsTransient = false;
	QString mImagePath;
	NotificationImage* mImagePixmap = nullptr;
	QString mDesktopEntry;
	QVariantMap mHints;
};

///! An action associated with a Notification.
/// See @@Notification.actions.
class NotificationAction: public QObject {
	Q_OBJECT;
	/// The identifier of the action.
	///
	/// When @@Notification.hasActionIcons is true, this property will be an icon name.
	/// When it is false, this property is irrelevant.
	Q_PROPERTY(QString identifier READ identifier CONSTANT);
	/// The localized text that should be displayed on a button.
	Q_PROPERTY(QString text READ text NOTIFY textChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("NotificationActions must be acquired from a Notification");

public:
	explicit NotificationAction(QString identifier, QString text, Notification* notification)
	    : QObject(notification)
	    , notification(notification)
	    , mIdentifier(std::move(identifier))
	    , mText(std::move(text)) {}

	/// Invoke the action. If @@Notification.resident is false it will be dismissed.
	Q_INVOKABLE void invoke();

	[[nodiscard]] QString identifier() const;
	[[nodiscard]] QString text() const;
	void setText(const QString& text);

signals:
	void textChanged();

private:
	Notification* notification;
	QString mIdentifier;
	QString mText;
};

} // namespace qs::service::notifications
