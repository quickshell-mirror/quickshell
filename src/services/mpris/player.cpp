#include "player.hpp"

#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_player.h"
#include "dbus_player_app.h"

using namespace qs::dbus;

namespace qs::service::mpris {

Q_LOGGING_CATEGORY(logMprisPlayer, "quickshell.service.mp.player", QtWarningMsg);

QString MprisPlaybackState::toString(MprisPlaybackState::Enum status) {
	switch (status) {
	case MprisPlaybackState::Stopped: return "Stopped";
	case MprisPlaybackState::Playing: return "Playing";
	case MprisPlaybackState::Paused: return "Paused";
	default: return "Unknown Status";
	}
}

QString MprisLoopState::toString(MprisLoopState::Enum status) {
	switch (status) {
	case MprisLoopState::None: return "None";
	case MprisLoopState::Track: return "Track";
	case MprisLoopState::Playlist: return "Playlist";
	default: return "Unknown Status";
	}
}

MprisPlayer::MprisPlayer(const QString& address, QObject* parent): QObject(parent) {
	this->app = new DBusMprisPlayerApp(
	    address,
	    "/org/mpris/MediaPlayer2",
	    QDBusConnection::sessionBus(),
	    this
	);

	this->player =
	    new DBusMprisPlayer(address, "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus(), this);

	if (!this->player->isValid() || !this->app->isValid()) {
		qCWarning(logMprisPlayer) << "Cannot create MprisPlayer for" << address;
		return;
	}

	// clang-format off
	QObject::connect(&this->pCanQuit, &AbstractDBusProperty::changed, this, &MprisPlayer::canQuitChanged);
	QObject::connect(&this->pCanRaise, &AbstractDBusProperty::changed, this, &MprisPlayer::canRaiseChanged);
	QObject::connect(&this->pCanSetFullscreen, &AbstractDBusProperty::changed, this, &MprisPlayer::canSetFullscreenChanged);
	QObject::connect(&this->pIdentity, &AbstractDBusProperty::changed, this, &MprisPlayer::identityChanged);
	QObject::connect(&this->pDesktopEntry, &AbstractDBusProperty::changed, this, &MprisPlayer::desktopEntryChanged);
	QObject::connect(&this->pFullscreen, &AbstractDBusProperty::changed, this, &MprisPlayer::fullscreenChanged);
	QObject::connect(&this->pSupportedUriSchemes, &AbstractDBusProperty::changed, this, &MprisPlayer::supportedUriSchemesChanged);
	QObject::connect(&this->pSupportedMimeTypes, &AbstractDBusProperty::changed, this, &MprisPlayer::supportedMimeTypesChanged);

	QObject::connect(&this->pCanControl, &AbstractDBusProperty::changed, this, &MprisPlayer::canControlChanged);
	QObject::connect(&this->pCanSeek, &AbstractDBusProperty::changed, this, &MprisPlayer::canSeekChanged);
	QObject::connect(&this->pCanGoNext, &AbstractDBusProperty::changed, this, &MprisPlayer::canGoNextChanged);
	QObject::connect(&this->pCanGoPrevious, &AbstractDBusProperty::changed, this, &MprisPlayer::canGoPreviousChanged);
	QObject::connect(&this->pCanPlay, &AbstractDBusProperty::changed, this, &MprisPlayer::canPlayChanged);
	QObject::connect(&this->pCanPause, &AbstractDBusProperty::changed, this, &MprisPlayer::canPauseChanged);

	QObject::connect(&this->pCanPlay, &AbstractDBusProperty::changed, this, &MprisPlayer::canTogglePlayingChanged);
	QObject::connect(&this->pCanPause, &AbstractDBusProperty::changed, this, &MprisPlayer::canTogglePlayingChanged);

	QObject::connect(&this->pPosition, &AbstractDBusProperty::changed, this, &MprisPlayer::onPositionChanged);
	QObject::connect(this->player, &DBusMprisPlayer::Seeked, this, &MprisPlayer::onSeek);
	QObject::connect(&this->pVolume, &AbstractDBusProperty::changed, this, &MprisPlayer::volumeChanged);
	QObject::connect(&this->pMetadata, &AbstractDBusProperty::changed, this, &MprisPlayer::onMetadataChanged);
	QObject::connect(&this->pPlaybackStatus, &AbstractDBusProperty::changed, this, &MprisPlayer::onPlaybackStatusChanged);
	QObject::connect(&this->pLoopStatus, &AbstractDBusProperty::changed, this, &MprisPlayer::onLoopStatusChanged);
	QObject::connect(&this->pRate, &AbstractDBusProperty::changed, this, &MprisPlayer::rateChanged);
	QObject::connect(&this->pMinRate, &AbstractDBusProperty::changed, this, &MprisPlayer::minRateChanged);
	QObject::connect(&this->pMaxRate, &AbstractDBusProperty::changed, this, &MprisPlayer::maxRateChanged);
	QObject::connect(&this->pShuffle, &AbstractDBusProperty::changed, this, &MprisPlayer::shuffleChanged);

	QObject::connect(&this->playerProperties, &DBusPropertyGroup::getAllFinished, this, &MprisPlayer::onGetAllFinished);

	// Ensure user triggered position updates can update length.
	QObject::connect(this, &MprisPlayer::positionChanged, this, &MprisPlayer::onExportedPositionChanged);
	// clang-format on

	this->appProperties.setInterface(this->app);
	this->playerProperties.setInterface(this->player);
	this->appProperties.updateAllViaGetAll();
	this->playerProperties.updateAllViaGetAll();
}

void MprisPlayer::raise() {
	if (!this->canRaise()) {
		qWarning() << "Cannot call raise() on" << this << "because canRaise is false.";
		return;
	}

	this->app->Raise();
}

void MprisPlayer::quit() {
	if (!this->canQuit()) {
		qWarning() << "Cannot call quit() on" << this << "because canQuit is false.";
		return;
	}

	this->app->Quit();
}

void MprisPlayer::openUri(const QString& uri) { this->player->OpenUri(uri); }

void MprisPlayer::next() {
	if (!this->canGoNext()) {
		qWarning() << "Cannot call next() on" << this << "because canGoNext is false.";
		return;
	}

	this->player->Next();
}

void MprisPlayer::previous() {
	if (!this->canGoPrevious()) {
		qWarning() << "Cannot call previous() on" << this << "because canGoPrevious is false.";
		return;
	}

	this->player->Previous();
}

void MprisPlayer::seek(qreal offset) {
	if (!this->canSeek()) {
		qWarning() << "Cannot call seek() on" << this << "because canSeek is false.";
		return;
	}

	auto target = static_cast<qlonglong>(offset * 1000) * 1000;
	this->player->Seek(target);
}

bool MprisPlayer::isValid() const { return this->player->isValid(); }
QString MprisPlayer::address() const { return this->player->service(); }

bool MprisPlayer::canControl() const { return this->pCanControl.get(); }
bool MprisPlayer::canPlay() const { return this->canControl() && this->pCanPlay.get(); }
bool MprisPlayer::canPause() const { return this->canControl() && this->pCanPause.get(); }

bool MprisPlayer::canTogglePlaying() const {
	if (this->mPlaybackState == MprisPlaybackState::Playing) return this->canPlay();
	else return this->canPause();
}

bool MprisPlayer::canSeek() const { return this->canControl() && this->pCanSeek.get(); }
bool MprisPlayer::canGoNext() const { return this->canControl() && this->pCanGoNext.get(); }
bool MprisPlayer::canGoPrevious() const { return this->canControl() && this->pCanGoPrevious.get(); }
bool MprisPlayer::canQuit() const { return this->pCanQuit.get(); }
bool MprisPlayer::canRaise() const { return this->pCanRaise.get(); }
bool MprisPlayer::canSetFullscreen() const { return this->pCanSetFullscreen.get(); }

QString MprisPlayer::identity() const { return this->pIdentity.get(); }
QString MprisPlayer::desktopEntry() const { return this->pDesktopEntry.get(); }

qlonglong MprisPlayer::positionMs() const {
	if (!this->positionSupported()) return 0; // unsupported
	if (this->mPlaybackState == MprisPlaybackState::Stopped) return 0;

	auto paused = this->mPlaybackState == MprisPlaybackState::Paused;
	auto time = paused ? this->pausedTime : QDateTime::currentDateTime();
	auto offset = time - this->lastPositionTimestamp;
	auto rateMul = static_cast<qlonglong>(this->pRate.get() * 1000);
	offset = (offset * rateMul) / 1000;

	return (this->pPosition.get() / 1000) + offset.count();
}

qreal MprisPlayer::position() const {
	if (!this->positionSupported()) return 0; // unsupported
	if (this->mPlaybackState == MprisPlaybackState::Stopped) return 0;

	return static_cast<qreal>(this->positionMs()) / 1000.0; // NOLINT
}

bool MprisPlayer::positionSupported() const { return this->pPosition.exists(); }

void MprisPlayer::setPosition(qreal position) {
	if (this->pPosition.get() == -1) {
		qWarning() << "Cannot set position of" << this << "because position is not supported.";
		return;
	}

	if (!this->canSeek()) {
		qWarning() << "Cannot set position of" << this << "because canSeek is false.";
		return;
	}

	auto target = static_cast<qlonglong>(position * 1000) * 1000;

	if (!this->mTrackId.isEmpty()) {
		this->player->SetPosition(QDBusObjectPath(this->mTrackId), target);
	} else {
		auto pos = this->positionMs() * 1000;
		this->player->Seek(target - pos);
	}

	this->pPosition.set(target);
}

void MprisPlayer::onPositionChanged() {
	const bool firstChange = !this->lastPositionTimestamp.isValid();
	this->lastPositionTimestamp = QDateTime::currentDateTimeUtc();
	emit this->positionChanged();
	if (firstChange) emit this->positionSupportedChanged();
}

void MprisPlayer::onExportedPositionChanged() {
	if (!this->lengthSupported()) emit this->lengthChanged();
}

void MprisPlayer::onSeek(qlonglong time) { this->pPosition.set(time); }

qreal MprisPlayer::length() const {
	if (this->mLength == -1) {
		return this->position(); // unsupported
	} else {
		return static_cast<qreal>(this->mLength / 1000) / 1000; // NOLINT
	}
}

bool MprisPlayer::lengthSupported() const { return this->mLength != -1; }

qreal MprisPlayer::volume() const { return this->pVolume.get(); }
bool MprisPlayer::volumeSupported() const { return this->pVolume.exists(); }

void MprisPlayer::setVolume(qreal volume) {
	if (!this->canControl()) {
		qWarning() << "Cannot set volume of" << this << "because canControl is false.";
		return;
	}

	if (!this->volumeSupported()) {
		qWarning() << "Cannot set volume of" << this << "because volume is not supported.";
		return;
	}

	this->pVolume.set(volume);
	this->pVolume.write();
}

QVariantMap MprisPlayer::metadata() const { return this->pMetadata.get(); }

void MprisPlayer::onMetadataChanged() {
	emit this->metadataChanged();

	auto lengthVariant = this->pMetadata.get().value("mpris:length");
	qlonglong length = -1;
	if (lengthVariant.isValid() && lengthVariant.canConvert<qlonglong>()) {
		length = lengthVariant.value<qlonglong>();
	}

	if (length != this->mLength) {
		this->mLength = length;
		emit this->lengthChanged();
	}

	auto trackChanged = false;

	auto trackidVariant = this->pMetadata.get().value("mpris:trackid");
	if (trackidVariant.isValid() && trackidVariant.canConvert<QString>()) {
		auto trackId = trackidVariant.value<QString>();

		if (trackId != this->mTrackId) {
			this->mTrackId = trackId;
			trackChanged = true;
		}
	}

	// Helps to catch players without trackid.
	auto urlVariant = this->pMetadata.get().value("xesam:url");
	if (urlVariant.isValid() && urlVariant.canConvert<QString>()) {
		auto url = urlVariant.value<QString>();

		if (url != this->mUrl) {
			this->mUrl = url;
			trackChanged = true;
		}
	}

	if (trackChanged) {
		// Some players don't seem to send position updates or seeks on track change.
		this->pPosition.update();
		emit this->trackChanged();
	}
}

MprisPlaybackState::Enum MprisPlayer::playbackState() const { return this->mPlaybackState; }

void MprisPlayer::setPlaybackState(MprisPlaybackState::Enum playbackState) {
	if (playbackState == this->mPlaybackState) return;

	switch (playbackState) {
	case MprisPlaybackState::Stopped:
		if (!this->canControl()) {
			qWarning() << "Cannot set playbackState of" << this
			           << "to Stopped because canControl is false.";
			return;
		}

		this->player->Stop();
		break;
	case MprisPlaybackState::Playing:
		if (!this->canPlay()) {
			qWarning() << "Cannot set playbackState of" << this << "to Playing because canPlay is false.";
			return;
		}

		this->player->Play();
		break;
	case MprisPlaybackState::Paused:
		if (!this->canPause()) {
			qWarning() << "Cannot set playbackState of" << this << "to Paused because canPause is false.";
			return;
		}

		this->player->Pause();
		break;
	default:
		qWarning() << "Cannot set playbackState of" << this << "to unknown value" << playbackState;
		return;
	}
}

void MprisPlayer::play() { this->setPlaybackState(MprisPlaybackState::Playing); }

void MprisPlayer::pause() { this->setPlaybackState(MprisPlaybackState::Paused); }

void MprisPlayer::stop() { this->setPlaybackState(MprisPlaybackState::Stopped); }

void MprisPlayer::togglePlaying() {
	if (this->mPlaybackState == MprisPlaybackState::Playing) {
		this->pause();
	} else {
		this->play();
	}
}

void MprisPlayer::onPlaybackStatusChanged() {
	const auto& status = this->pPlaybackStatus.get();

	auto state = MprisPlaybackState::Stopped;
	if (status == "Playing") {
		state = MprisPlaybackState::Playing;
	} else if (status == "Paused") {
		this->pausedTime = QDateTime::currentDateTimeUtc();
		state = MprisPlaybackState::Paused;
	} else if (status == "Stopped") {
		state = MprisPlaybackState::Stopped;
	} else {
		state = MprisPlaybackState::Stopped;
		qWarning() << "Received unexpected PlaybackStatus for" << this << status;
	}

	if (state != this->mPlaybackState) {
		// make sure we're in sync at least on play/pause. Some players don't automatically send this.
		this->pPosition.update();
		this->mPlaybackState = state;
		emit this->playbackStateChanged();
	}
}

MprisLoopState::Enum MprisPlayer::loopState() const { return this->mLoopState; }
bool MprisPlayer::loopSupported() const { return this->pLoopStatus.exists(); }

void MprisPlayer::setLoopState(MprisLoopState::Enum loopState) {
	if (!this->canControl()) {
		qWarning() << "Cannot set loopState of" << this << "because canControl is false.";
		return;
	}

	if (!this->loopSupported()) {
		qWarning() << "Cannot set loopState of" << this << "because loop state is not supported.";
		return;
	}

	if (loopState == this->mLoopState) return;

	QString loopStatusStr;
	switch (loopState) {
	case MprisLoopState::None: loopStatusStr = "None"; break;
	case MprisLoopState::Track: loopStatusStr = "Track"; break;
	case MprisLoopState::Playlist: loopStatusStr = "Playlist"; break;
	default:
		qWarning() << "Cannot set loopState of" << this << "to unknown value" << loopState;
		return;
	}

	this->pLoopStatus.set(loopStatusStr);
	this->pLoopStatus.write();
}

void MprisPlayer::onLoopStatusChanged() {
	const auto& status = this->pLoopStatus.get();

	if (status == "None") {
		this->mLoopState = MprisLoopState::None;
	} else if (status == "Track") {
		this->mLoopState = MprisLoopState::Track;
	} else if (status == "Playlist") {
		this->mLoopState = MprisLoopState::Playlist;
	} else {
		this->mLoopState = MprisLoopState::None;
		qWarning() << "Received unexpected LoopStatus for" << this << status;
	}

	emit this->loopStateChanged();
}

qreal MprisPlayer::rate() const { return this->pRate.get(); }
qreal MprisPlayer::minRate() const { return this->pMinRate.get(); }
qreal MprisPlayer::maxRate() const { return this->pMaxRate.get(); }

void MprisPlayer::setRate(qreal rate) {
	if (rate == this->pRate.get()) return;

	if (rate < this->pMinRate.get() || rate > this->pMaxRate.get()) {
		qWarning() << "Cannot set rate for" << this << "to" << rate
		           << "which is outside of minRate and maxRate" << this->pMinRate.get()
		           << this->pMaxRate.get();
		return;
	}

	this->pRate.set(rate);
	this->pRate.write();
}

bool MprisPlayer::shuffle() const { return this->pShuffle.get(); }
bool MprisPlayer::shuffleSupported() const { return this->pShuffle.exists(); }

void MprisPlayer::setShuffle(bool shuffle) {
	if (!this->shuffleSupported()) {
		qWarning() << "Cannot set shuffle for" << this << "because shuffle is not supported.";
		return;
	}

	if (!this->canControl()) {
		qWarning() << "Cannot set shuffle state of" << this << "because canControl is false.";
		return;
	}

	this->pShuffle.set(shuffle);
	this->pShuffle.write();
}

bool MprisPlayer::fullscreen() const { return this->pFullscreen.get(); }

void MprisPlayer::setFullscreen(bool fullscreen) {
	if (!this->canSetFullscreen()) {
		qWarning() << "Cannot set fullscreen for" << this << "because canSetFullscreen is false.";
		return;
	}

	this->pFullscreen.set(fullscreen);
	this->pFullscreen.write();
}

QList<QString> MprisPlayer::supportedUriSchemes() const { return this->pSupportedUriSchemes.get(); }
QList<QString> MprisPlayer::supportedMimeTypes() const { return this->pSupportedMimeTypes.get(); }

void MprisPlayer::onGetAllFinished() {
	if (this->volumeSupported()) emit this->volumeSupportedChanged();
	if (this->loopSupported()) emit this->loopSupportedChanged();
	if (this->shuffleSupported()) emit this->shuffleSupportedChanged();
	emit this->ready();
}

} // namespace qs::service::mpris
