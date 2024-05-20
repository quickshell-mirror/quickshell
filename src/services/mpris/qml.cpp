#include "qml.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "player.hpp"
#include "watcher.hpp"

using namespace qs::dbus;
using namespace qs::service::mp;

Player::Player(qs::service::mp::MprisPlayer* player, QObject* parent)
    : QObject(parent)
    , player(player) {

	// clang-format off
	QObject::connect(&this->player->canControl, &AbstractDBusProperty::changed, this, &Player::canControlChanged);
	QObject::connect(&this->player->canGoNext, &AbstractDBusProperty::changed, this, &Player::canGoNextChanged);
	QObject::connect(&this->player->canGoPrevious, &AbstractDBusProperty::changed, this, &Player::canGoPreviousChanged);
	QObject::connect(&this->player->canPlay, &AbstractDBusProperty::changed, this, &Player::canPlayChanged);
	QObject::connect(&this->player->canPause, &AbstractDBusProperty::changed, this, &Player::canPauseChanged);
	QObject::connect(&this->player->metadata, &AbstractDBusProperty::changed, this, &Player::metadataChanged);
	QObject::connect(&this->player->playbackStatus, &AbstractDBusProperty::changed, this, &Player::playbackStatusChanged);	
	QObject::connect(&this->player->position, &AbstractDBusProperty::changed, this, &Player::positionChanged);
	QObject::connect(&this->player->minimumRate, &AbstractDBusProperty::changed, this, &Player::positionChanged);
	QObject::connect(&this->player->maximumRate, &AbstractDBusProperty::changed, this, &Player::positionChanged);

	QObject::connect(&this->player->loopStatus, &AbstractDBusProperty::changed, this, &Player::loopStatusChanged);
	QObject::connect(&this->player->rate, &AbstractDBusProperty::changed, this, &Player::rateChanged); 
	QObject::connect(&this->player->shuffle, &AbstractDBusProperty::changed, this, &Player::shuffleChanged); 
	QObject::connect(&this->player->volume, &AbstractDBusProperty::changed, this, &Player::volumeChanged);
	// clang-format on
}

bool Player::canControl() const {
	if (this->player == nullptr) return false;
	return this->player->canControl.get();
}

bool Player::canGoNext() const {
	if (this->player == nullptr) return false;
	return this->player->canGoNext.get();
}

bool Player::canGoPrevious() const {
	if (this->player == nullptr) return false;
	return this->player->canGoPrevious.get();
}

bool Player::canPlay() const {
	if (this->player == nullptr) return false;
	return this->player->canPlay.get();
}

bool Player::canPause() const {
	if (this->player == nullptr) return false;
	return this->player->canPause.get();
}

QVariantMap Player::metadata() const {
	if (this->player == nullptr) return {};
	return this->player->metadata.get();
}

QString Player::playbackStatus() const {
	if (this->player == nullptr) return "";

	if (this->player->playbackStatus.get().isEmpty()) return "Unsupported";
	return this->player->playbackStatus.get();
}

qlonglong Player::position() const {
	if (this->player == nullptr) return 0;
	return this->player->position.get();
}

double Player::minimumRate() const {
	if (this->player == nullptr) return 0.0;
	return this->player->minimumRate.get();
}

double Player::maximumRate() const {
	if (this->player == nullptr) return 0.0;
	return this->player->maximumRate.get();
}

QString Player::loopStatus() const {
	if (this->player == nullptr) return "";

	if (this->player->loopStatus.get().isEmpty()) return "Unsupported";
	return this->player->loopStatus.get();
}

double Player::rate() const {
	if (this->player == nullptr) return 0.0;
	return this->player->rate.get();
}

bool Player::shuffle() const {
	if (this->player == nullptr) return false;
	return this->player->shuffle.get();
}

double Player::volume() const {
	if (this->player == nullptr) return 0.0;
	return this->player->volume.get();
}

// NOLINTBEGIN
void Player::setPosition(QDBusObjectPath trackId, qlonglong position) const {
	this->player->setPosition(trackId, position);
}
void Player::next() const { this->player->next(); }
void Player::previous() const { this->player->previous(); }
void Player::pause() const { this->player->pause(); }
void Player::playPause() const { this->player->playPause(); }
void Player::stop() const { this->player->stop(); }
void Player::play() const { this->player->play(); }
// NOLINTEND

Mpris::Mpris(QObject* parent): QObject(parent) {
	auto* watcher = MprisWatcher::instance();

	// clang-format off
	QObject::connect(watcher, &MprisWatcher::MprisPlayerRegistered, this, &Mpris::onPlayerRegistered);
	QObject::connect(watcher, &MprisWatcher::MprisPlayerUnregistered, this, &Mpris::onPlayerUnregistered);
	// clang-format on

	for (QString& player: watcher->players) {
		this->mPlayers.push_back(new Player(new MprisPlayer(player), this));
	}
}

void Mpris::onPlayerRegistered(const QString& service) {
	this->mPlayers.push_back(new Player(new MprisPlayer(service), this));
	emit this->playersChanged();
}

void Mpris::onPlayerUnregistered(const QString& service) {
	Player* mprisPlayer = nullptr;
	MprisPlayer* player = playerWithAddress(players(), service)->player;

	this->mPlayers.removeIf([player, &mprisPlayer](Player* testPlayer) {
		if (testPlayer->player == player) {
			mprisPlayer = testPlayer;
			return true;
		} else return false;
	});

	emit this->playersChanged();

	delete mprisPlayer->player;
	delete mprisPlayer;
}

QQmlListProperty<Player> Mpris::players() {
	return QQmlListProperty<Player>(this, nullptr, &Mpris::playersCount, &Mpris::playerAt);
}

qsizetype Mpris::playersCount(QQmlListProperty<Player>* property) {
	return reinterpret_cast<Mpris*>(property->object)->mPlayers.count(); // NOLINT
}

Player* Mpris::playerAt(QQmlListProperty<Player>* property, qsizetype index) {
	return reinterpret_cast<Mpris*>(property->object)->mPlayers.at(index); // NOLINT
}

Player* Mpris::playerWithAddress(QQmlListProperty<Player> property, const QString& address) {
	for (Player* player: reinterpret_cast<Mpris*>(property.object)->mPlayers) { // NOLINT
		if (player->player->watcherId == address) {
			return player;
		}
	}

	return nullptr;
}
