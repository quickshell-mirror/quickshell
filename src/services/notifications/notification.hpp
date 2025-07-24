#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/retainable.hpp"
#include "../../core/util.hpp"
#include "dbusimage.hpp"

namespace qs::service::notifications {

///! The urgency level of a Notification.
/// See @@Notification.urgency.
class NotificationUrgency: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Low = 0,
		Normal = 1,
		Critical = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(qs::service::notifications::NotificationUrgency::Enum value);
};

///! The reason a Notification was closed.
/// See @@Notification.closed(s).
class NotificationCloseReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// The notification expired due to a timeout.
		Expired = 1,
		/// The notification was explicitly dismissed by the user.
		Dismissed = 2,
		/// The remote application requested the notification be removed.
		CloseRequested = 3,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString
	toString(qs::service::notifications::NotificationCloseReason::Enum value);
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
	// clang-format off
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
	Q_PROPERTY(qreal expireTimeout READ default NOTIFY expireTimeoutChanged BINDABLE bindableExpireTimeout);
	/// The sending application's name.
	Q_PROPERTY(QString appName READ default NOTIFY appNameChanged BINDABLE bindableAppName);
	/// The sending application's icon. If none was provided, then the icon from an associated
	/// desktop entry will be retrieved. If none was found then "".
	Q_PROPERTY(QString appIcon READ default NOTIFY appIconChanged BINDABLE bindableAppIcon);
	/// The image associated with this notification, or "" if none.
	Q_PROPERTY(QString summary READ default NOTIFY summaryChanged BINDABLE bindableSummary);
	Q_PROPERTY(QString body READ default NOTIFY bodyChanged BINDABLE bindableBody);
	Q_PROPERTY(qs::service::notifications::NotificationUrgency::Enum urgency READ default NOTIFY urgencyChanged BINDABLE bindableUrgency);
	/// Actions that can be taken for this notification.
	Q_PROPERTY(QList<qs::service::notifications::NotificationAction*> actions READ actions NOTIFY actionsChanged);
	/// If actions associated with this notification have icons available.
	///
	/// See @@NotificationAction.identifier for details.
	Q_PROPERTY(bool hasActionIcons READ default NOTIFY hasActionIconsChanged BINDABLE bindableHasActionIcons);
	/// If true, the notification will not be destroyed after an action is invoked.
	Q_PROPERTY(bool resident READ default NOTIFY residentChanged BINDABLE bindableResident);
	/// If true, the notification should skip any kind of persistence function like a notification area.
	Q_PROPERTY(bool transient READ default NOTIFY transientChanged BINDABLE bindableTransient);
	/// The name of the sender's desktop entry or "" if none was supplied.
	Q_PROPERTY(QString desktopEntry READ default NOTIFY desktopEntryChanged BINDABLE bindableDesktopEntry);
	/// An image associated with the notification.
	///
	/// This image is often something like a profile picture in instant messaging applications.
	Q_PROPERTY(QString image READ default NOTIFY imageChanged BINDABLE bindableImage);
	/// If true, the notification has an inline reply action.
	/// 
	/// A quick reply text field should be displayed and the reply can be sent using @@sendInlineReply().
	Q_PROPERTY(bool hasInlineReply READ default NOTIFY hasInlineReplyChanged BINDABLE bindableHasInlineReply);
	/// The placeholder text/button caption for the inline reply.
	Q_PROPERTY(QString inlineReplyPlaceholder READ default NOTIFY inlineReplyPlaceholderChanged BINDABLE bindableInlineReplyPlaceholder);
	/// All hints sent by the client application as a javascript object.
	/// Many common hints are exposed via other properties.
	Q_PROPERTY(QVariantMap hints READ default NOTIFY hintsChanged BINDABLE bindableHints);
	// clang-format on
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

	/// Send an inline reply to the notification with an inline reply action.
	/// > [!WARNING] This method can only be called if
	/// > @@hasInlineReply is true
	/// > and the server has @@NotificationServer.inlineReplySupported set to true.
	Q_INVOKABLE void sendInlineReply(const QString& replyText);

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

	[[nodiscard]] bool isLastGeneration() const;
	void setLastGeneration();

	[[nodiscard]] QBindable<qreal> bindableExpireTimeout() const { return &this->bExpireTimeout; }
	[[nodiscard]] QBindable<QString> bindableAppName() const { return &this->bAppName; }
	[[nodiscard]] QBindable<QString> bindableAppIcon() const { return &this->bAppIcon; }
	[[nodiscard]] QBindable<QString> bindableSummary() const { return &this->bSummary; }
	[[nodiscard]] QBindable<QString> bindableBody() const { return &this->bBody; }
	[[nodiscard]] QBindable<NotificationUrgency ::Enum> bindableUrgency() const {
		return &this->bUrgency;
	}

	[[nodiscard]] QList<NotificationAction*> actions() const;

	[[nodiscard]] QBindable<bool> bindableHasActionIcons() const { return &this->bHasActionIcons; }
	[[nodiscard]] QBindable<bool> bindableResident() const { return &this->bResident; }
	[[nodiscard]] QBindable<bool> bindableTransient() const { return &this->bTransient; }
	[[nodiscard]] QBindable<QString> bindableDesktopEntry() const { return &this->bDesktopEntry; }
	[[nodiscard]] QBindable<QString> bindableImage() const { return &this->bImage; }
	[[nodiscard]] QBindable<bool> bindableHasInlineReply() const { return &this->bHasInlineReply; }

	[[nodiscard]] QBindable<QString> bindableInlineReplyPlaceholder() const {
		return &this->bInlineReplyPlaceholder;
	}

	[[nodiscard]] QBindable<QVariantMap> bindableHints() const { return &this->bHints; }

	[[nodiscard]] NotificationCloseReason::Enum closeReason() const;
	void setTracked(bool tracked);

signals:
	/// Sent when a notification has been closed.
	///
	/// The notification object will be destroyed as soon as all signal handlers exit.
	void closed(qs::service::notifications::NotificationCloseReason::Enum reason);

	void trackedChanged();
	void expireTimeoutChanged();
	void appNameChanged();
	void appIconChanged();
	void summaryChanged();
	void bodyChanged();
	void urgencyChanged();
	void actionsChanged();
	void hasActionIconsChanged();
	void residentChanged();
	void transientChanged();
	void desktopEntryChanged();
	void imageChanged();
	void hasInlineReplyChanged();
	void inlineReplyPlaceholderChanged();
	void hintsChanged();

private:
	quint32 mId;
	NotificationCloseReason::Enum mCloseReason = NotificationCloseReason::Dismissed;
	bool mLastGeneration = false;
	NotificationImage mImagePixmap;
	QList<NotificationAction*> mActions;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Notification, qreal, bExpireTimeout, &Notification::expireTimeoutChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bAppName, &Notification::appNameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bAppIcon, &Notification::appIconChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bSummary, &Notification::summaryChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bBody, &Notification::bodyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, bool, bHasActionIcons, &Notification::hasActionIconsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, bool, bResident, &Notification::residentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, bool, bTransient, &Notification::transientChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bDesktopEntry, &Notification::desktopEntryChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bImage, &Notification::imageChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, bool, bHasInlineReply, &Notification::hasInlineReplyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QString, bInlineReplyPlaceholder, &Notification::inlineReplyPlaceholderChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Notification, QVariantMap, bHints, &Notification::hintsChanged);
	// clang-format on

	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(
	    Notification,
	    NotificationUrgency::Enum,
	    bUrgency,
	    NotificationUrgency::Normal,
	    &Notification::urgencyChanged
	);
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
