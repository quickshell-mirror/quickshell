#pragma once

#include <QtCore/qtmetamacros.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../../core/doc.hpp"
#include "../../core/model.hpp"
#include "../../core/reload.hpp"
#include "notification.hpp"
#include "server.hpp"

namespace qs::service::notifications {

///! Desktop Notifications Server.
/// An implementation of the [Desktop Notifications Specification] for receiving notifications
/// from external applications.
///
/// The server does not advertise most capabilities by default. See the individual properties for details.
///
/// [Desktop Notifications Specification]: https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html
class NotificationServerQml: public PostReloadHook {
	Q_OBJECT;
	// clang-format off
	/// If notifications should be re-emitted when quickshell reloads. Defaults to true.
	///
	/// The @@Notification.lastGeneration flag will be
	/// set on notifications from the prior generation for further filtering/handling.
	Q_PROPERTY(bool keepOnReload READ keepOnReload WRITE setKeepOnReload NOTIFY keepOnReloadChanged);
	/// If the notification server should advertise that it can persist notifications in the background
	/// after going offscreen. Defaults to false.
	Q_PROPERTY(bool persistenceSupported READ persistenceSupported WRITE setPersistenceSupported NOTIFY persistenceSupportedChanged);
	/// If notification body text should be advertised as supported by the notification server.
	/// Defaults to true.
	///
	/// Note that returned notifications are likely to return body text even if this property is false,
	/// as it is only a hint.
	Q_PROPERTY(bool bodySupported READ bodySupported WRITE setBodySupported NOTIFY bodySupportedChanged);
	/// If notification body text should be advertised as supporting markup as described in [the specification]
	/// Defaults to false.
	///
	/// Note that returned notifications may still contain markup if this property is false,
	/// as it is only a hint. By default Text objects will try to render markup. To avoid this
	/// if any is sent, change @@QtQuick.Text.textFormat to `PlainText`.
	Q_PROPERTY(bool bodyMarkupSupported READ bodyMarkupSupported WRITE setBodyMarkupSupported NOTIFY bodyMarkupSupportedChanged);
	/// If notification body text should be advertised as supporting hyperlinks as described in [the specification]
	/// Defaults to false.
	///
	/// Note that returned notifications may still contain hyperlinks if this property is false, as it is only a hint.
	///
	/// [the specification]: https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html#hyperlinks
	Q_PROPERTY(bool bodyHyperlinksSupported READ bodyHyperlinksSupported WRITE setBodyHyperlinksSupported NOTIFY bodyHyperlinksSupportedChanged);
	/// If notification body text should be advertised as supporting images as described in [the specification]
	/// Defaults to false.
	///
	/// Note that returned notifications may still contain images if this property is false, as it is only a hint.
	///
	/// [the specification]: https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html#images
	Q_PROPERTY(bool bodyImagesSupported READ bodyImagesSupported WRITE setBodyImagesSupported NOTIFY bodyImagesSupportedChanged);
	/// If notification actions should be advertised as supported by the notification server. Defaults to false.
	Q_PROPERTY(bool actionsSupported READ actionsSupported WRITE setActionsSupported NOTIFY actionsSupportedChanged);
	/// If notification actions should be advertised as supporting the display of icons. Defaults to false.
	Q_PROPERTY(bool actionIconsSupported READ actionIconsSupported WRITE setActionIconsSupported NOTIFY actionIconsSupportedChanged);
	/// If the notification server should advertise that it supports images. Defaults to false.
	Q_PROPERTY(bool imageSupported READ imageSupported WRITE setImageSupported NOTIFY imageSupportedChanged);
	/// If the notification server should advertise that it supports inline replies. Defaults to false.
	Q_PROPERTY(bool inlineReplySupported READ inlineReplySupported WRITE setInlineReplySupported NOTIFY inlineReplySupportedChanged);
	/// All notifications currently tracked by the server.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::notifications::Notification>*);
	Q_PROPERTY(UntypedObjectModel* trackedNotifications READ trackedNotifications NOTIFY trackedNotificationsChanged);
	/// Extra hints to expose to notification clients.
	Q_PROPERTY(QVector<QString> extraHints READ extraHints WRITE setExtraHints NOTIFY extraHintsChanged);
	// clang-format on
	QML_NAMED_ELEMENT(NotificationServer);

public:
	void onPostReload() override;

	[[nodiscard]] bool keepOnReload() const;
	void setKeepOnReload(bool keepOnReload);

	[[nodiscard]] bool persistenceSupported() const;
	void setPersistenceSupported(bool persistenceSupported);

	[[nodiscard]] bool bodySupported() const;
	void setBodySupported(bool bodySupported);

	[[nodiscard]] bool bodyMarkupSupported() const;
	void setBodyMarkupSupported(bool bodyMarkupSupported);

	[[nodiscard]] bool bodyHyperlinksSupported() const;
	void setBodyHyperlinksSupported(bool bodyHyperlinksSupported);

	[[nodiscard]] bool bodyImagesSupported() const;
	void setBodyImagesSupported(bool bodyImagesSupported);

	[[nodiscard]] bool actionsSupported() const;
	void setActionsSupported(bool actionsSupported);

	[[nodiscard]] bool actionIconsSupported() const;
	void setActionIconsSupported(bool actionIconsSupported);

	[[nodiscard]] bool imageSupported() const;
	void setImageSupported(bool imageSupported);

	[[nodiscard]] bool inlineReplySupported() const;
	void setInlineReplySupported(bool inlineReplySupported);

	[[nodiscard]] QVector<QString> extraHints() const;
	void setExtraHints(QVector<QString> extraHints);

	[[nodiscard]] ObjectModel<Notification>* trackedNotifications() const;

signals:
	/// Sent when a notification is received by the server.
	///
	/// If this notification should not be discarded, set its `tracked` property to true.
	void notification(qs::service::notifications::Notification* notification);

	void keepOnReloadChanged();
	void persistenceSupportedChanged();
	void bodySupportedChanged();
	void bodyMarkupSupportedChanged();
	void bodyHyperlinksSupportedChanged();
	void bodyImagesSupportedChanged();
	void actionsSupportedChanged();
	void actionIconsSupportedChanged();
	void imageSupportedChanged();
	void inlineReplySupportedChanged();
	void extraHintsChanged();
	void trackedNotificationsChanged();

private:
	void updateSupported();

	bool live = false;
	bool mKeepOnReload = true;
	NotificationServerSupport support;
};

} // namespace qs::service::notifications
