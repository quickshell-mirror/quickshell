#include "player.hpp"

#include <qdbusextratypes.h>
#include <qdbusmetatype.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qsize.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_player.h"

using namespace qs::dbus;

Q_LOGGING_CATEGORY(logMprisPlayer, "quickshell.service.mp.player", QtWarningMsg);

namespace qs::service::mp {

MprisPlayer::MprisPlayer(const QString& address, QObject* parent)
    : QObject(parent)
    , watcherId(address) {
	// qDBusRegisterMetaType<DBusSniIconPixmap>();
	// qDBusRegisterMetaType<DBusSniIconPixmapList>();
	// qDBusRegisterMetaType<DBusSniTooltip>();
	// spec is unclear about what exactly an item address is, so account for both
	auto splitIdx = address.indexOf('/');
	auto conn = splitIdx == -1 ? address : address.sliced(0, splitIdx);
	auto path = splitIdx == -1 ? "/org/mpris/MediaPlayer2" : address.sliced(splitIdx);

	this->player = new DBusMprisPlayer(conn, path, QDBusConnection::sessionBus(), this);

	if (!this->player->isValid()) {
		qCWarning(logMprisPlayer).noquote() << "Cannot create MprisPlayer for" << conn;
		return;
	}

	// clang-format off
	QObject::connect(&this->canControl, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->canGoNext, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->canGoPrevious, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->canPlay, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->canPause, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->metadata, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer); 	
	QObject::connect(&this->playbackStatus, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);	
	QObject::connect(&this->position, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->minimumRate, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->maximumRate, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer); 

	QObject::connect(&this->loopStatus, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->rate, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->shuffle, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);
	QObject::connect(&this->volume, &AbstractDBusProperty::changed, this, &MprisPlayer::updatePlayer);

	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFinished, this, &MprisPlayer::onGetAllFinished);
	// clang-format on

	this->properties.setInterface(this->player);
	this->properties.updateAllViaGetAll();
}

bool MprisPlayer::isValid() const { return this->player->isValid(); }
bool MprisPlayer::isReady() const { return this->mReady; }

void MprisPlayer::setPosition(QDBusObjectPath trackId, qlonglong position) { // NOLINT
	this->player->SetPosition(trackId, position);
}
void MprisPlayer::next() { this->player->Next(); }
void MprisPlayer::previous() { this->player->Previous(); }
void MprisPlayer::pause() { this->player->Pause(); }
void MprisPlayer::playPause() { this->player->PlayPause(); }
void MprisPlayer::stop() { this->player->Stop(); }
void MprisPlayer::play() { this->player->Play(); }

void MprisPlayer::onGetAllFinished() {
	if (this->mReady) return;
	this->mReady = true;
	emit this->ready();
}

void MprisPlayer::updatePlayer() { // NOLINT
	                                 // TODO: emit signal here
}

} // namespace qs::service::mp
