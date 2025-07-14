#include "notification.hpp"

#include <qcontainerfwd.h>
#include <qdbusargument.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/desktopentry.hpp"
#include "../../core/iconimageprovider.hpp"
#include "../../core/logcat.hpp"
#include "dbusimage.hpp"
#include "server.hpp"

namespace qs::service::notifications {

// NOLINTNEXTLINE(misc-use-internal-linkage)
QS_DECLARE_LOGGING_CATEGORY(logNotifications); // server.cpp

QString NotificationUrgency::toString(NotificationUrgency::Enum value) {
	switch (value) {
	case NotificationUrgency::Low: return "Low";
	case NotificationUrgency::Normal: return "Normal";
	case NotificationUrgency::Critical: return "Critical";
	default: return "Invalid notification urgency";
	}
}

QString NotificationCloseReason::toString(NotificationCloseReason::Enum value) {
	switch (value) {
	case NotificationCloseReason::Expired: return "Expired";
	case NotificationCloseReason::Dismissed: return "Dismissed";
	case NotificationCloseReason::CloseRequested: return "CloseRequested";
	default: return "Invalid notification close reason";
	}
}

QString NotificationAction::identifier() const { return this->mIdentifier; }
QString NotificationAction::text() const { return this->mText; }

void NotificationAction::invoke() {
	if (this->notification->isRetained()) {
		qCritical() << "Cannot invoke destroyed notification" << this;
		return;
	}

	NotificationServer::instance()->ActionInvoked(this->notification->id(), this->mIdentifier);

	if (!this->notification->bindableResident().value()) {
		this->notification->close(NotificationCloseReason::Dismissed);
	}
}

void NotificationAction::setText(const QString& text) {
	if (text != this->mText) return;

	this->mText = text;
	emit this->textChanged();
}

void Notification::expire() { this->close(NotificationCloseReason::Expired); }
void Notification::dismiss() { this->close(NotificationCloseReason::Dismissed); }

void Notification::close(NotificationCloseReason::Enum reason) {
	if (this->isRetained()) {
		qCritical() << "Cannot close destroyed notification" << this;
		return;
	}

	this->mCloseReason = reason;

	if (reason != 0) {
		NotificationServer::instance()->deleteNotification(this, reason);
	}
}

void Notification::sendInlineReply(const QString& replyText) {
	if (!NotificationServer::instance()->support.inlineReply) {
		qCritical() << "Inline reply support disabled on server";
		return;
	}

	if (!this->bHasInlineReply) {
		qCritical() << "Cannot send reply to notification without inline-reply action";
		return;
	}

	if (this->isRetained()) {
		qCritical() << "Cannot send reply to destroyed notification" << this;
		return;
	}

	NotificationServer::instance()->NotificationReplied(this->id(), replyText);

	if (!this->bindableResident().value()) {
		this->close(NotificationCloseReason::Dismissed);
	}
}

void Notification::updateProperties(
    const QString& appName,
    QString appIcon,
    const QString& summary,
    const QString& body,
    const QStringList& actions,
    QVariantMap hints,
    qint32 expireTimeout
) {
	Qt::beginPropertyUpdateGroup();

	this->bExpireTimeout = expireTimeout;
	this->bAppName = appName;
	this->bSummary = summary;
	this->bBody = body;
	this->bHasActionIcons = hints.value("action-icons").toBool();
	this->bResident = hints.value("resident").toBool();
	this->bTransient = hints.value("transient").toBool();
	this->bDesktopEntry = hints.value("desktop-entry").toString();

	this->bUrgency = hints.contains("urgency")
	                   ? hints.value("urgency").value<NotificationUrgency::Enum>()
	                   : NotificationUrgency::Normal;

	if (appIcon.isEmpty() && !this->bDesktopEntry.value().isEmpty()) {
		if (auto* entry = DesktopEntryManager::instance()->byId(this->bDesktopEntry.value())) {
			appIcon = entry->mIcon;
		}
	}

	this->bAppIcon = appIcon;

	QString imageDataName;
	if (hints.contains("image-data")) imageDataName = "image-data";
	else if (hints.contains("image_data")) imageDataName = "image_data";
	else if (hints.contains("icon_data")) imageDataName = "icon_data";

	QString imagePath;

	if (imageDataName.isEmpty()) {
		this->mImagePixmap.clear();
	} else {
		auto value = hints.value(imageDataName).value<QDBusArgument>();
		value >> this->mImagePixmap.writeImage();
		imagePath = this->mImagePixmap.url();
	}

	// don't store giant byte arrays longer than necessary
	hints.remove("image-data");
	hints.remove("image_data");
	hints.remove("icon_data");

	if (!this->mImagePixmap.hasData()) {
		QString imagePathName;
		if (hints.contains("image-path")) imagePathName = "image-path";
		else if (hints.contains("image_path")) imagePathName = "image_path";

		if (!imagePathName.isEmpty()) {
			imagePath = hints.value(imagePathName).value<QString>();

			if (!imagePath.startsWith("file:")) {
				imagePath = IconImageProvider::requestString(imagePath, "");
			}
		}
	}

	this->bImage = imagePath;
	this->bHints = hints;

	bool actionsChanged = false;
	auto deletedActions = QVector<NotificationAction*>();

	if (actions.length() % 2 == 0) {
		int ai = 0;
		for (auto i = 0; i != actions.length(); i += 2) {
			const auto& identifier = actions.at(i);
			const auto& text = actions.at(i + 1);

			if (identifier == "inline-reply" && NotificationServer::instance()->support.inlineReply) {
				if (this->bHasInlineReply) {
					qCWarning(logNotifications) << this << '(' << appName << ')'
					                            << "sent an action set with duplicate inline-reply actions.";
				} else {
					this->bHasInlineReply = true;
					this->bInlineReplyPlaceholder = text;
				}
				// skip inserting this action into action list
				continue;
			}

			auto* action = ai < this->mActions.length() ? this->mActions.at(ai) : nullptr;

			if (action && identifier == action->identifier()) {
				action->setText(text);
			} else {
				auto* newAction = new NotificationAction(identifier, text, this);

				if (action) {
					deletedActions.push_back(action);
					this->mActions.replace(ai, newAction);
				} else {
					this->mActions.push_back(newAction);
				}

				actionsChanged = true;
			}

			ai++;
		}

		for (auto i = this->mActions.length(); i > ai; i--) {
			deletedActions.push_back(this->mActions.at(i - 1));
			this->mActions.remove(i - 1);
			actionsChanged = true;
		}
	} else {
		qCWarning(logNotifications) << this << '(' << appName << ')'
		                            << "sent an action set of an invalid length.";
	}

	Qt::endPropertyUpdateGroup();

	if (actionsChanged) emit this->actionsChanged();

	for (auto* action: deletedActions) {
		delete action;
	}
}

quint32 Notification::id() const { return this->mId; }
bool Notification::isTracked() const { return this->mCloseReason == 0; }
QList<NotificationAction*> Notification::actions() const { return this->mActions; }
NotificationCloseReason::Enum Notification::closeReason() const { return this->mCloseReason; }

void Notification::setTracked(bool tracked) {
	this->close(
	    tracked ? static_cast<NotificationCloseReason::Enum>(0) : NotificationCloseReason::Dismissed
	);
}

bool Notification::isLastGeneration() const { return this->mLastGeneration; }
void Notification::setLastGeneration() { this->mLastGeneration = true; }

} // namespace qs::service::notifications
