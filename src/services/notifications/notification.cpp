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
#include "../../core/util.hpp"
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
	if (this->notification->isRetained()) {
		qCritical() << "Cannot invoke destroyed notification" << this;
		return;
	}

	NotificationServer::instance()->ActionInvoked(this->notification->id(), this->mIdentifier);

	if (!this->notification->resident()) {
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
	                                         : static_cast<quint8>(NotificationUrgency::Normal);

	auto hasActionIcons = hints.value("action-icons").value<bool>();
	auto resident = hints.value("resident").value<bool>();
	auto transient = hints.value("transient").value<bool>();
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

	auto expireTimeoutChanged = this->setExpireTimeout(expireTimeout);
	auto appNameChanged = this->setAppName(appName);
	auto appIconChanged = this->setAppIcon(appIcon);
	auto summaryChanged = this->setSummary(summary);
	auto bodyChanged = this->setBody(body);
	auto urgencyChanged = this->setUrgency(static_cast<NotificationUrgency::Enum>(urgency));
	auto hasActionIconsChanged = this->setHasActionIcons(hasActionIcons);
	auto residentChanged = this->setResident(resident);
	auto transientChanged = this->setTransient(transient);
	auto desktopEntryChanged = this->setDesktopEntry(desktopEntry);
	DEFINE_DROP_EMIT_IF(imagePixmap || imagePath != this->mImagePath, this, imageChanged);
	auto hintsChanged = this->setHints(hints);

	NotificationImage* oldImage = nullptr;

	if (imageChanged) {
		oldImage = this->mImagePixmap;
		this->mImagePixmap = imagePixmap;
		this->mImagePath = imagePath;
	}

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

	DropEmitter::call(
	    expireTimeoutChanged,
	    appNameChanged,
	    appIconChanged,
	    summaryChanged,
	    bodyChanged,
	    urgencyChanged,
	    hasActionIconsChanged,
	    residentChanged,
	    transientChanged,
	    desktopEntryChanged,
	    imageChanged,
	    hintsChanged
	);

	if (actionsChanged) emit this->actionsChanged();

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

DEFINE_MEMBER_GETSET(Notification, expireTimeout, setExpireTimeout);
DEFINE_MEMBER_GETSET(Notification, appName, setAppName);
DEFINE_MEMBER_GETSET(Notification, appIcon, setAppIcon);
DEFINE_MEMBER_GETSET(Notification, summary, setSummary);
DEFINE_MEMBER_GETSET(Notification, body, setBody);
DEFINE_MEMBER_GETSET(Notification, urgency, setUrgency);
DEFINE_MEMBER_GET(Notification, actions);
DEFINE_MEMBER_GETSET(Notification, hasActionIcons, setHasActionIcons);
DEFINE_MEMBER_GETSET(Notification, resident, setResident);
DEFINE_MEMBER_GETSET(Notification, transient, setTransient);
DEFINE_MEMBER_GETSET(Notification, desktopEntry, setDesktopEntry);

QString Notification::image() const {
	if (this->mImagePixmap) {
		return this->mImagePixmap->url();
	} else {
		return this->mImagePath;
	}
}

DEFINE_MEMBER_GETSET(Notification, hints, setHints);

} // namespace qs::service::notifications
