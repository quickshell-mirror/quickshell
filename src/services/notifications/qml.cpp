#include "qml.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../../core/model.hpp"
#include "notification.hpp"
#include "server.hpp"

namespace qs::service::notifications {

void NotificationServerQml::onPostReload() {
	auto* instance = NotificationServer::instance();
	instance->support = this->support;

	QObject::connect(
	    instance,
	    &NotificationServer::notification,
	    this,
	    &NotificationServerQml::notification
	);

	instance->switchGeneration(this->mKeepOnReload, [this]() {
		this->live = true;
		emit this->trackedNotificationsChanged();
	});
}

bool NotificationServerQml::keepOnReload() const { return this->mKeepOnReload; }

void NotificationServerQml::setKeepOnReload(bool keepOnReload) {
	if (keepOnReload == this->mKeepOnReload) return;

	if (this->live) {
		qCritical() << "Cannot set NotificationServer.keepOnReload after the server has been started.";
		return;
	}

	this->mKeepOnReload = keepOnReload;
	emit this->keepOnReloadChanged();
}

bool NotificationServerQml::persistenceSupported() const { return this->support.persistence; }

void NotificationServerQml::setPersistenceSupported(bool persistenceSupported) {
	if (persistenceSupported == this->support.persistence) return;
	this->support.persistence = persistenceSupported;
	this->updateSupported();
	emit this->persistenceSupportedChanged();
}

bool NotificationServerQml::bodySupported() const { return this->support.body; }

void NotificationServerQml::setBodySupported(bool bodySupported) {
	if (bodySupported == this->support.body) return;
	this->support.body = bodySupported;
	this->updateSupported();
	emit this->bodySupportedChanged();
}

bool NotificationServerQml::bodyMarkupSupported() const { return this->support.bodyMarkup; }

void NotificationServerQml::setBodyMarkupSupported(bool bodyMarkupSupported) {
	if (bodyMarkupSupported == this->support.bodyMarkup) return;
	this->support.bodyMarkup = bodyMarkupSupported;
	this->updateSupported();
	emit this->bodyMarkupSupportedChanged();
}

bool NotificationServerQml::bodyHyperlinksSupported() const { return this->support.bodyHyperlinks; }

void NotificationServerQml::setBodyHyperlinksSupported(bool bodyHyperlinksSupported) {
	if (bodyHyperlinksSupported == this->support.bodyHyperlinks) return;
	this->support.bodyHyperlinks = bodyHyperlinksSupported;
	this->updateSupported();
	emit this->bodyHyperlinksSupportedChanged();
}

bool NotificationServerQml::bodyImagesSupported() const { return this->support.bodyImages; }

void NotificationServerQml::setBodyImagesSupported(bool bodyImagesSupported) {
	if (bodyImagesSupported == this->support.bodyImages) return;
	this->support.bodyImages = bodyImagesSupported;
	this->updateSupported();
	emit this->bodyImagesSupportedChanged();
}

bool NotificationServerQml::actionsSupported() const { return this->support.actions; }

void NotificationServerQml::setActionsSupported(bool actionsSupported) {
	if (actionsSupported == this->support.actions) return;
	this->support.actions = actionsSupported;
	this->updateSupported();
	emit this->actionsSupportedChanged();
}

bool NotificationServerQml::actionIconsSupported() const { return this->support.actionIcons; }

void NotificationServerQml::setActionIconsSupported(bool actionIconsSupported) {
	if (actionIconsSupported == this->support.actionIcons) return;
	this->support.actionIcons = actionIconsSupported;
	this->updateSupported();
	emit this->actionIconsSupportedChanged();
}

bool NotificationServerQml::imageSupported() const { return this->support.image; }

void NotificationServerQml::setImageSupported(bool imageSupported) {
	if (imageSupported == this->support.image) return;
	this->support.image = imageSupported;
	this->updateSupported();
	emit this->imageSupportedChanged();
}

bool NotificationServerQml::inlineReplySupported() const { return this->support.inlineReply; }

void NotificationServerQml::setInlineReplySupported(bool inlineReplySupported) {
	if (inlineReplySupported == this->support.inlineReply) return;
	this->support.inlineReply = inlineReplySupported;
	this->updateSupported();
	emit this->inlineReplySupportedChanged();
}

QVector<QString> NotificationServerQml::extraHints() const { return this->support.extraHints; }

void NotificationServerQml::setExtraHints(QVector<QString> extraHints) {
	if (extraHints == this->support.extraHints) return;
	this->support.extraHints = std::move(extraHints);
	this->updateSupported();
	emit this->extraHintsChanged();
}

ObjectModel<Notification>* NotificationServerQml::trackedNotifications() const {
	if (this->live) {
		return NotificationServer::instance()->trackedNotifications();
	} else {
		return ObjectModel<Notification>::emptyInstance();
	}
}

void NotificationServerQml::updateSupported() {
	if (this->live) {
		NotificationServer::instance()->support = this->support;
	}
}

} // namespace qs::service::notifications
