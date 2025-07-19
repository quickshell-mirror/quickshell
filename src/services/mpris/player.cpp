#include "player.hpp"

#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qdbusextratypes.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_player.h"
#include "dbus_player_app.h"

using namespace qs::dbus;

namespace qs::service::mpris {

namespace {
QS_LOGGING_CATEGORY(logMprisPlayer, "quickshell.service.mp.player", QtWarningMsg);
}

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

	this->bCanPlay.setBinding([this]() { return this->bCanControl && this->bpCanPlay; });
	this->bCanPause.setBinding([this]() { return this->bCanControl && this->bpCanPause; });
	this->bCanSeek.setBinding([this]() { return this->bCanControl && this->bpCanSeek; });
	this->bCanGoNext.setBinding([this]() { return this->bCanControl && this->bpCanGoNext; });
	this->bCanGoPrevious.setBinding([this]() { return this->bCanControl && this->bpCanGoPrevious; });

	this->bCanTogglePlaying.setBinding([this]() {
		return this->bIsPlaying ? this->bCanPause.value() : this->bCanPlay.value();
	});

	this->bTrackTitle.setBinding([this]() {
		return this->bMetadata.value().value("xesam:title").toString();
	});

	this->bTrackAlbum.setBinding([this]() {
		return this->bMetadata.value().value("xesam:album").toString();
	});

	this->bTrackArtist.setBinding([this]() {
		const auto& artist = this->bMetadata.value().value("xesam:artist").value<QList<QString>>();
		return artist.isEmpty() ? QString() : artist.join(", ");
	});

	this->bTrackAlbumArtist.setBinding([this]() {
		return this->bMetadata.value().value("xesam:albumArtist").toString();
	});

	this->bTrackArtUrl.setBinding([this]() {
		return this->bMetadata.value().value("mpris:artUrl").toString();
	});

	this->bInternalLength.setBinding([this]() {
		auto variant = this->bMetadata.value().value("mpris:length");
		if (variant.isValid() && variant.canConvert<qlonglong>()) {
			return variant.value<qlonglong>();
		} else return static_cast<qlonglong>(-1);
	});

	this->bLengthSupported.setBinding([this]() { return this->bInternalLength != -1; });

	this->bIsPlaying.setBinding([this]() {
		return this->bPlaybackState == MprisPlaybackState::Playing;
	});

	// clang-format off
	QObject::connect(this->player, &DBusMprisPlayer::Seeked, this, &MprisPlayer::onSeek);
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
	if (!this->bCanRaise) {
		qWarning() << "Cannot call raise() on" << this << "because canRaise is false.";
		return;
	}

	this->app->Raise();
}

void MprisPlayer::quit() {
	if (!this->bCanQuit) {
		qWarning() << "Cannot call quit() on" << this << "because canQuit is false.";
		return;
	}

	this->app->Quit();
}

void MprisPlayer::openUri(const QString& uri) { this->player->OpenUri(uri); }

void MprisPlayer::next() {
	if (!this->bCanGoNext) {
		qWarning() << "Cannot call next() on" << this << "because canGoNext is false.";
		return;
	}

	this->player->Next();
}

void MprisPlayer::previous() {
	if (!this->bCanGoPrevious) {
		qWarning() << "Cannot call previous() on" << this << "because canGoPrevious is false.";
		return;
	}

	this->player->Previous();
}

void MprisPlayer::seek(qreal offset) {
	if (!this->bCanSeek) {
		qWarning() << "Cannot call seek() on" << this << "because canSeek is false.";
		return;
	}

	auto target = static_cast<qlonglong>(offset * 1000) * 1000;
	this->player->Seek(target);
}

bool MprisPlayer::isValid() const { return this->player->isValid(); }
QString MprisPlayer::address() const { return this->player->service(); }

qlonglong MprisPlayer::positionMs() const {
	if (!this->positionSupported()) return 0; // unsupported
	if (this->bPlaybackState == MprisPlaybackState::Stopped) return 0;

	auto paused = this->bPlaybackState == MprisPlaybackState::Paused;
	auto time = paused ? this->pausedTime : QDateTime::currentDateTime();
	auto offset = time - this->lastPositionTimestamp;
	auto rateMul = static_cast<qlonglong>(this->bRate.value() * 1000);
	offset = (offset * rateMul) / 1000;

	return (this->bpPosition.value() / 1000) + offset.count();
}

qreal MprisPlayer::position() const {
	if (!this->positionSupported()) return 0; // unsupported
	if (this->bPlaybackState == MprisPlaybackState::Stopped) return 0;

	return static_cast<qreal>(this->positionMs()) / 1000.0; // NOLINT
}

bool MprisPlayer::positionSupported() const { return this->pPosition.exists(); }

void MprisPlayer::setPosition(qreal position) {
	if (this->bpPosition.value() == -1) {
		qWarning() << "Cannot set position of" << this << "because position is not supported.";
		return;
	}

	if (!this->bCanSeek) {
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

	this->setPosition(target);
}

void MprisPlayer::onPositionUpdated() {
	const bool firstChange = !this->lastPositionTimestamp.isValid();
	this->lastPositionTimestamp = QDateTime::currentDateTimeUtc();
	this->pausedTime = this->lastPositionTimestamp;
	emit this->positionChanged();
	if (firstChange) emit this->positionSupportedChanged();
}

void MprisPlayer::setPosition(qlonglong position) {
	this->bpPosition = position;
	this->onPositionUpdated();
}

void MprisPlayer::onExportedPositionChanged() {
	if (!this->bLengthSupported) emit this->lengthChanged();
}

void MprisPlayer::onSeek(qlonglong time) { this->setPosition(time); }

qreal MprisPlayer::length() const {
	if (!this->bLengthSupported) {
		return this->position(); // unsupported
	} else {
		return static_cast<qreal>(this->bInternalLength / 1000) / 1000; // NOLINT
	}
}

bool MprisPlayer::volumeSupported() const { return this->pVolume.exists(); }

void MprisPlayer::setVolume(qreal volume) {
	if (!this->bCanControl) {
		qWarning() << "Cannot set volume of" << this << "because canControl is false.";
		return;
	}

	if (!this->volumeSupported()) {
		qWarning() << "Cannot set volume of" << this << "because volume is not supported.";
		return;
	}

	this->bVolume = volume;
	this->pVolume.write();
}

void MprisPlayer::onMetadataChanged() {
	auto trackChanged = false;

	QString trackId;
	auto trackidVariant = this->bpMetadata.value().value("mpris:trackid");
	if (trackidVariant.isValid()) {
		if (trackidVariant.canConvert<QString>()) {
			trackId = trackidVariant.toString();
		} else if (trackidVariant.canConvert<QDBusObjectPath>()) {
			trackId = trackidVariant.value<QDBusObjectPath>().path();
		}
	}

	if (trackId != this->mTrackId) {
		this->mTrackId = trackId;
		trackChanged = true;
	}

	// Helps to catch players without trackid.
	auto urlVariant = this->bpMetadata.value().value("xesam:url");
	if (urlVariant.isValid() && urlVariant.canConvert<QString>()) {
		auto url = urlVariant.toString();

		if (url != this->mTrackUrl) {
			this->mTrackUrl = url;
			trackChanged = true;
		}
	}

	// Some players (Jellyfin) specify xesam:url or mpris:trackid
	// and DON'T ACTUALLY CHANGE THEM WHEN THE TRACK CHANGES.
	auto titleVariant = this->bpMetadata.value().value("xesam:title");
	if (titleVariant.isValid() && titleVariant.canConvert<QString>()) {
		auto title = titleVariant.toString();

		if (title != this->mTrackTitle) {
			this->mTrackTitle = title;
			trackChanged = true;
		}
	}

	Qt::beginPropertyUpdateGroup();

	if (trackChanged) {
		emit this->trackChanged();
		this->bUniqueId = this->bUniqueId + 1;
		this->trackChangedBeforeState = true;

		// Some players don't seem to send position updates or seeks on track change.
		this->pPosition.requestUpdate();
	}

	this->bMetadata = this->bpMetadata.value();

	Qt::endPropertyUpdateGroup();

	if (trackChanged) emit this->postTrackChanged();
}

void MprisPlayer::setPlaybackState(MprisPlaybackState::Enum playbackState) {
	if (playbackState == this->bPlaybackState) return;

	switch (playbackState) {
	case MprisPlaybackState::Stopped:
		if (!this->bCanControl) {
			qWarning() << "Cannot set playbackState of" << this
			           << "to Stopped because canControl is false.";
			return;
		}

		this->player->Stop();
		break;
	case MprisPlaybackState::Playing:
		if (!this->bCanPlay) {
			qWarning() << "Cannot set playbackState of" << this << "to Playing because canPlay is false.";
			return;
		}

		this->player->Play();
		break;
	case MprisPlaybackState::Paused:
		if (!this->bCanPause) {
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
	if (this->bIsPlaying) {
		this->pause();
	} else {
		this->play();
	}
}

void MprisPlayer::setPlaying(bool playing) {
	if (playing == this->bIsPlaying) return;
	this->togglePlaying();
}

void MprisPlayer::onPlaybackStatusUpdated() {
	// Insurance - have not yet seen a player where this particular check is required that doesn't
	// require the late query below.
	this->pPosition.requestUpdate();

	// For exceptionally bad players that update playback timestamps at an indeterminate time AFTER
	// updating playback state. (Youtube)
	QTimer::singleShot(100, this, [&]() { this->pPosition.requestUpdate(); });

	// For exceptionally bad players that don't update length (or other metadata) until a new track actually
	// starts playing, and then don't trigger a metadata update when they do. (Jellyfin)
	if (this->trackChangedBeforeState) {
		this->trackChangedBeforeState = false;
		this->pMetadata.requestUpdate();
	}
}

bool MprisPlayer::loopSupported() const { return this->pLoopStatus.exists(); }

void MprisPlayer::setLoopState(MprisLoopState::Enum loopState) {
	if (!this->bCanControl) {
		qWarning() << "Cannot set loopState of" << this << "because canControl is false.";
		return;
	}

	if (!this->loopSupported()) {
		qWarning() << "Cannot set loopState of" << this << "because loop state is not supported.";
		return;
	}

	if (loopState == this->bLoopState) return;
	if (loopState < MprisLoopState::None || loopState > MprisLoopState::Playlist) {
		qWarning() << "Cannot set loopState of" << this << "to unknown value" << loopState;
	}

	this->bLoopState = loopState;
	this->pLoopStatus.write();
}

void MprisPlayer::setRate(qreal rate) {
	if (rate == this->bRate.value()) return;

	if (rate < this->bMinRate.value() || rate > this->bMaxRate.value()) {
		qWarning() << "Cannot set rate for" << this << "to" << rate
		           << "which is outside of minRate and maxRate" << this->bMinRate.value()
		           << this->bMaxRate.value();
		return;
	}

	this->bRate = rate;
	this->pRate.write();
}

bool MprisPlayer::shuffleSupported() const { return this->pShuffle.exists(); }

void MprisPlayer::setShuffle(bool shuffle) {
	if (!this->shuffleSupported()) {
		qWarning() << "Cannot set shuffle for" << this << "because shuffle is not supported.";
		return;
	}

	if (!this->bCanControl) {
		qWarning() << "Cannot set shuffle state of" << this << "because canControl is false.";
		return;
	}

	this->bShuffle = shuffle;
	this->pShuffle.write();
}

void MprisPlayer::setFullscreen(bool fullscreen) {
	if (!this->bCanSetFullscreen) {
		qWarning() << "Cannot set fullscreen for" << this << "because canSetFullscreen is false.";
		return;
	}

	this->bFullscreen = fullscreen;
	this->pFullscreen.write();
}

void MprisPlayer::onGetAllFinished() {
	if (this->volumeSupported()) emit this->volumeSupportedChanged();
	if (this->loopSupported()) emit this->loopSupportedChanged();
	if (this->shuffleSupported()) emit this->shuffleSupportedChanged();
	emit this->ready();
}

} // namespace qs::service::mpris

namespace qs::dbus {

using namespace qs::service::mpris;

DBusResult<MprisPlaybackState::Enum>
DBusDataTransform<MprisPlaybackState::Enum>::fromWire(const QString& wire) {
	if (wire == "Playing") return MprisPlaybackState::Playing;
	if (wire == "Paused") return MprisPlaybackState::Paused;
	if (wire == "Stopped") return MprisPlaybackState::Stopped;
	return QDBusError(QDBusError::InvalidArgs, QString("Invalid MprisPlaybackState: %1").arg(wire));
}

QString DBusDataTransform<MprisPlaybackState::Enum>::toWire(MprisPlaybackState::Enum data) {
	switch (data) {
	case MprisPlaybackState::Playing: return "Playing";
	case MprisPlaybackState::Paused: return "Paused";
	case MprisPlaybackState::Stopped: return "Stopped";
	default: qFatal() << "Tried to convert an invalid MprisPlaybackState to String"; return QString();
	}
}

DBusResult<MprisLoopState::Enum>
DBusDataTransform<MprisLoopState::Enum>::fromWire(const QString& wire) {
	if (wire == "None") return MprisLoopState::None;
	if (wire == "Track") return MprisLoopState::Track;
	if (wire == "Playlist") return MprisLoopState::Playlist;
	return QDBusError(QDBusError::InvalidArgs, QString("Invalid MprisLoopState: %1").arg(wire));
}

QString DBusDataTransform<MprisLoopState::Enum>::toWire(MprisLoopState::Enum data) {
	switch (data) {
	case MprisLoopState::None: return "None";
	case MprisLoopState::Track: return "Track";
	case MprisLoopState::Playlist: return "Playlist";
	default: qFatal() << "Tried to convert an invalid MprisLoopState to String"; return QString();
	}
}

} // namespace qs::dbus
