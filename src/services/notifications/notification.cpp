#include "notification.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qdbusargument.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/desktopentry.hpp"
#include "../../core/iconimageprovider.hpp"
#include "dbusimage.hpp"
#include "server.hpp"

namespace qs::service::notifications {

Q_DECLARE_LOGGING_CATEGORY(logNotifications); // server.cpp

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
	NotificationServer::instance()->ActionInvoked(this->notification->id(), this->mIdentifier);

	if (!this->notification->isResident()) {
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
	this->mCloseReason = reason;

	if (reason != 0) {
		NotificationServer::instance()->deleteNotification(this, reason);
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
	auto urgency = hints.contains("urgency") ? hints.value("urgency").value<quint8>()
	                                         : NotificationUrgency::Normal;

	auto hasActionIcons = hints.value("action-icons").value<bool>();
	auto isResident = hints.value("resident").value<bool>();
	auto isTransient = hints.value("transient").value<bool>();
	auto desktopEntry = hints.value("desktop-entry").value<QString>();

	QString imageDataName;
	if (hints.contains("image-data")) imageDataName = "image-data";
	else if (hints.contains("image_data")) imageDataName = "image_data";
	else if (hints.contains("icon_data")) imageDataName = "icon_data";

	NotificationImage* imagePixmap = nullptr;
	if (!imageDataName.isEmpty()) {
		auto value = hints.value(imageDataName).value<QDBusArgument>();
		DBusNotificationImage image;
		value >> image;
		imagePixmap = new NotificationImage(std::move(image), this);
	}

	// don't store giant byte arrays more than necessary
	hints.remove("image-data");
	hints.remove("image_data");
	hints.remove("icon_data");

	QString imagePath;
	if (!imagePixmap) {
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

	if (appIcon.isEmpty() && !desktopEntry.isEmpty()) {
		if (auto* entry = DesktopEntryManager::instance()->byId(desktopEntry)) {
			appIcon = entry->mIcon;
		}
	}

	auto appNameChanged = appName != this->mAppName;
	auto appIconChanged = appIcon != this->mAppIcon;
	auto summaryChanged = summary != this->mSummary;
	auto bodyChanged = body != this->mBody;
	auto expireTimeoutChanged = expireTimeout != this->mExpireTimeout;
	auto urgencyChanged = urgency != this->mUrgency;
	auto hasActionIconsChanged = hasActionIcons != this->mHasActionIcons;
	auto isResidentChanged = isResident != this->mIsResident;
	auto isTransientChanged = isTransient != this->mIsTransient;
	auto desktopEntryChanged = desktopEntry != this->mDesktopEntry;
	auto imageChanged = imagePixmap || imagePath != this->mImagePath;
	auto hintsChanged = hints != this->mHints;

	if (appNameChanged) this->mAppName = appName;
	if (appIconChanged) this->mAppIcon = appIcon;
	if (summaryChanged) this->mSummary = summary;
	if (bodyChanged) this->mBody = body;
	if (expireTimeoutChanged) this->mExpireTimeout = expireTimeout;
	if (urgencyChanged) this->mUrgency = static_cast<NotificationUrgency::Enum>(urgency);
	if (hasActionIcons) this->mHasActionIcons = hasActionIcons;
	if (isResidentChanged) this->mIsResident = isResident;
	if (isTransientChanged) this->mIsTransient = isTransient;
	if (desktopEntryChanged) this->mDesktopEntry = desktopEntry;

	NotificationImage* oldImage = nullptr;

	if (imageChanged) {
		oldImage = this->mImagePixmap;
		this->mImagePixmap = imagePixmap;
		this->mImagePath = imagePath;
	}

	if (hintsChanged) this->mHints = hints;

	bool actionsChanged = false;
	auto deletedActions = QVector<NotificationAction*>();

	if (actions.length() % 2 == 0) {
		int ai = 0;
		for (auto i = 0; i != actions.length(); i += 2) {
			ai = i / 2;
			const auto& identifier = actions.at(i);
			const auto& text = actions.at(i + 1);
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

	if (appNameChanged) emit this->appNameChanged();
	if (appIconChanged) emit this->appIconChanged();
	if (summaryChanged) emit this->summaryChanged();
	if (bodyChanged) emit this->bodyChanged();
	if (expireTimeoutChanged) emit this->expireTimeoutChanged();
	if (urgencyChanged) emit this->urgencyChanged();
	if (actionsChanged) emit this->actionsChanged();
	if (hasActionIconsChanged) emit this->hasActionIconsChanged();
	if (isResidentChanged) emit this->isResidentChanged();
	if (isTransientChanged) emit this->isTransientChanged();
	if (desktopEntryChanged) emit this->desktopEntryChanged();
	if (imageChanged) emit this->imageChanged();
	if (hintsChanged) emit this->hintsChanged();

	for (auto* action: deletedActions) {
		delete action;
	}

	delete oldImage;
}

quint32 Notification::id() const { return this->mId; }
bool Notification::isTracked() const { return this->mCloseReason == 0; }
NotificationCloseReason::Enum Notification::closeReason() const { return this->mCloseReason; }

void Notification::setTracked(bool tracked) {
	this->close(
	    tracked ? static_cast<NotificationCloseReason::Enum>(0) : NotificationCloseReason::Dismissed
	);
}

bool Notification::isLastGeneration() const { return this->mLastGeneration; }
void Notification::setLastGeneration() { this->mLastGeneration = true; }

qreal Notification::expireTimeout() const { return this->mExpireTimeout; }
QString Notification::appName() const { return this->mAppName; }
QString Notification::appIcon() const { return this->mAppIcon; }
QString Notification::summary() const { return this->mSummary; }
QString Notification::body() const { return this->mBody; }
NotificationUrgency::Enum Notification::urgency() const { return this->mUrgency; }
QVector<NotificationAction*> Notification::actions() const { return this->mActions; }
bool Notification::hasActionIcons() const { return this->mHasActionIcons; }
bool Notification::isResident() const { return this->mIsResident; }
bool Notification::isTransient() const { return this->mIsTransient; }
QString Notification::desktopEntry() const { return this->mDesktopEntry; }

QString Notification::image() const {
	if (this->mImagePixmap) {
		return this->mImagePixmap->url();
	} else {
		return this->mImagePath;
	}
}

QVariantMap Notification::hints() const { return this->mHints; }

} // namespace qs::service::notifications
