#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/doc.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_player.h"
#include "dbus_player_app.h"

namespace qs::service::mpris {

class MprisPlaybackState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		Stopped = 0,
		Playing = 1,
		Paused = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(MprisPlaybackState::Enum status);
};

class MprisLoopState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		None = 0,
		Track = 1,
		Playlist = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(MprisLoopState::Enum status);
};

///! A media player exposed over MPRIS.
/// A media player exposed over MPRIS.
///
/// > [!WARNING] Support for various functionality and general compliance to
/// > the MPRIS specification varies wildly by player.
/// > Always check the associated `canXyz` and `xyzSupported` properties if available.
///
/// > [!INFO] The TrackList and Playlist interfaces were not implemented as we could not
/// > find any media players using them to test against.
class MprisPlayer: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(bool canControl READ canControl NOTIFY canControlChanged);
	Q_PROPERTY(bool canPlay READ canPlay NOTIFY canPlayChanged);
	Q_PROPERTY(bool canPause READ canPause NOTIFY canPauseChanged);
	Q_PROPERTY(bool canSeek READ canSeek NOTIFY canSeekChanged);
	Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY canGoNextChanged);
	Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY canGoPreviousChanged);
	Q_PROPERTY(bool canQuit READ canQuit NOTIFY canQuitChanged);
	Q_PROPERTY(bool canRaise READ canRaise NOTIFY canRaiseChanged);
	Q_PROPERTY(bool canSetFullscreen READ canSetFullscreen NOTIFY canSetFullscreenChanged);
	/// The human readable name of the media player.
	Q_PROPERTY(QString identity READ identity NOTIFY identityChanged);
	/// The name of the desktop entry for the media player, or an empty string if not provided.
	Q_PROPERTY(QString desktopEntry READ desktopEntry NOTIFY desktopEntryChanged);
	/// The current position in the playing track, as seconds, with millisecond precision,
	/// or `0` if `positionSupported` is false.
	///
	/// May only be written to if `canSeek` and `positionSupported` are true.
	///
	/// > [!WARNING] To avoid excessive property updates wasting CPU while `position` is not
	/// > actively monitored, `position` usually will not update reactively, unless a nonlinear
	/// > change in position occurs, however reading it will always return the current position.
	/// >
	/// > If you want to actively monitor the position, the simplest way it to emit the `positionChanged`
	/// > signal manually for the duration you are monitoring it, Using a [FrameAnimation] if you need
	/// > the value to update smoothly, such as on a slider, or a [Timer] if not, as shown below.
	/// >
	/// > ```qml {filename="Using a FrameAnimation"}
	/// > FrameAnimation {
	/// >   // only emit the signal when the position is actually changing.
	/// >   running: player.playbackState == MprisPlaybackState.Playing
	/// >   // emit the positionChanged signal every frame.
	/// >   onTriggered: player.positionChanged()
	/// > }
	/// > ```
	/// >
	/// > ```qml {filename="Using a Timer"}
	/// > Timer {
	/// >   // only emit the signal when the position is actually changing.
	/// >   running: player.playbackState == MprisPlaybackState.Playing
	/// >   // Make sure the position updates at least once per second.
	/// >   interval: 1000
	/// >   repeat: true
	/// >   // emit the positionChanged signal every second.
	/// >   onTriggered: player.positionChanged()
	/// > }
	/// > ```
	///
	/// [FrameAnimation]: https://doc.qt.io/qt-6/qml-qtquick-frameanimation.html
	/// [Timer]: https://doc.qt.io/qt-6/qml-qtqml-timer.html
	Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged);
	Q_PROPERTY(bool positionSupported READ positionSupported NOTIFY positionSupportedChanged);
	/// The length of the playing track, as seconds, with millisecond precision,
	/// or the value of `position` if `lengthSupported` is false.
	Q_PROPERTY(qreal length READ length NOTIFY lengthChanged);
	Q_PROPERTY(bool lengthSupported READ lengthSupported NOTIFY lengthSupportedChanged);
	/// The volume of the playing track from 0.0 to 1.0, or 1.0 if `volumeSupported` is false.
	///
	/// May only be written to if `canControl` and `volumeSupported` are true.
	Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged);
	Q_PROPERTY(bool volumeSupported READ volumeSupported NOTIFY volumeSupportedChanged);
	/// Metadata of the current track.
	///
	/// A map of common properties is available [here](https://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata).
	/// Do not count on any of them actually being present.
	Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged);
	/// The playback state of the media player.
	///
	/// - If `canPlay` is false, you cannot assign the `Playing` state.
	/// - If `canPause` is false, you cannot assign the `Paused` state.
	/// - If `canControl` is false, you cannot assign the `Stopped` state.
	/// (or any of the others, though their repsective properties will also be false)
	Q_PROPERTY(MprisPlaybackState::Enum playbackState READ playbackState WRITE setPlaybackState NOTIFY playbackStateChanged);
	/// The loop state of the media player, or `None` if `loopSupported` is false.
	///
	/// May only be written to if `canControl` and `loopSupported` are true.
	Q_PROPERTY(MprisLoopState::Enum loopState READ loopState WRITE setLoopState NOTIFY loopStateChanged);
	Q_PROPERTY(bool loopSupported READ loopSupported NOTIFY loopSupportedChanged);
	/// The speed the song is playing at, as a multiplier.
	///
	/// Only values between `minRate` and `maxRate` (inclusive) may be written to the property.
	/// Additionally, It is recommended that you only write common values such as `0.25`, `0.5`, `1.0`, `2.0`
	/// to the property, as media players are free to ignore the value, and are more likely to
	/// accept common ones.
	Q_PROPERTY(qreal rate READ rate WRITE setRate NOTIFY rateChanged);
	Q_PROPERTY(qreal minRate READ minRate NOTIFY minRateChanged);
	Q_PROPERTY(qreal maxRate READ maxRate NOTIFY maxRateChanged);
	/// If the play queue is currently being shuffled, or false if `shuffleSupported` is false.
	///
	/// May only be written if `canControl` and `shuffleSupported` are true.
	Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged);
	Q_PROPERTY(bool shuffleSupported READ shuffleSupported NOTIFY shuffleSupportedChanged);
	/// If the player is currently shown in fullscreen.
	///
	/// May only be written to if `canSetFullscreen` is true.
	Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged);
	/// Uri schemes supported by `openUri`.
	Q_PROPERTY(QList<QString> supportedUriSchemes READ supportedUriSchemes NOTIFY supportedUriSchemesChanged);
	/// Mime types supported by `openUri`.
	Q_PROPERTY(QList<QString> supportedMimeTypes READ supportedMimeTypes NOTIFY supportedMimeTypesChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("MprisPlayers can only be acquired from Mpris");

public:
	explicit MprisPlayer(const QString& address, QObject* parent = nullptr);

	/// Bring the media player to the front of the window stack.
	///
	/// May only be called if `canRaise` is true.
	Q_INVOKABLE void raise();
	/// Quit the media player.
	///
	/// May only be called if `canQuit` is true.
	Q_INVOKABLE void quit();
	/// Open the given URI in the media player.
	///
	/// Many players will silently ignore this, especially if the uri
	/// does not match `supportedUriSchemes` and `supportedMimeTypes`.
	Q_INVOKABLE void openUri(const QString& uri);
	/// Play the next song.
	///
	/// May only be called if `canGoNext` is true.
	Q_INVOKABLE void next();
	/// Play the previous song, or go back to the beginning of the current one.
	///
	/// May only be called if `canGoPrevious` is true.
	Q_INVOKABLE void previous();
	/// Change `position` by an offset.
	///
	/// Even if `positionSupported` is false and you cannot set `position`,
	/// this function may work.
	///
	/// May only be called if `canSeek` is true.
	Q_INVOKABLE void seek(qreal offset);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString address() const;

	[[nodiscard]] bool canControl() const;
	[[nodiscard]] bool canSeek() const;
	[[nodiscard]] bool canGoNext() const;
	[[nodiscard]] bool canGoPrevious() const;
	[[nodiscard]] bool canPlay() const;
	[[nodiscard]] bool canPause() const;
	[[nodiscard]] bool canQuit() const;
	[[nodiscard]] bool canRaise() const;
	[[nodiscard]] bool canSetFullscreen() const;

	[[nodiscard]] QString identity() const;
	[[nodiscard]] QString desktopEntry() const;

	[[nodiscard]] qlonglong positionMs() const;
	[[nodiscard]] qreal position() const;
	[[nodiscard]] bool positionSupported() const;
	void setPosition(qreal position);

	[[nodiscard]] qreal length() const;
	[[nodiscard]] bool lengthSupported() const;

	[[nodiscard]] qreal volume() const;
	[[nodiscard]] bool volumeSupported() const;
	void setVolume(qreal volume);

	[[nodiscard]] QVariantMap metadata() const;

	[[nodiscard]] MprisPlaybackState::Enum playbackState() const;
	void setPlaybackState(MprisPlaybackState::Enum playbackState);

	[[nodiscard]] MprisLoopState::Enum loopState() const;
	[[nodiscard]] bool loopSupported() const;
	void setLoopState(MprisLoopState::Enum loopState);

	[[nodiscard]] qreal rate() const;
	[[nodiscard]] qreal minRate() const;
	[[nodiscard]] qreal maxRate() const;
	void setRate(qreal rate);

	[[nodiscard]] bool shuffle() const;
	[[nodiscard]] bool shuffleSupported() const;
	void setShuffle(bool shuffle);

	[[nodiscard]] bool fullscreen() const;
	void setFullscreen(bool fullscreen);

	[[nodiscard]] QList<QString> supportedUriSchemes() const;
	[[nodiscard]] QList<QString> supportedMimeTypes() const;

signals:
	void trackChanged();

	QSDOC_HIDE void ready();
	void canControlChanged();
	void canPlayChanged();
	void canPauseChanged();
	void canSeekChanged();
	void canGoNextChanged();
	void canGoPreviousChanged();
	void canQuitChanged();
	void canRaiseChanged();
	void canSetFullscreenChanged();
	void identityChanged();
	void desktopEntryChanged();
	void positionChanged();
	void positionSupportedChanged();
	void lengthChanged();
	void lengthSupportedChanged();
	void volumeChanged();
	void volumeSupportedChanged();
	void metadataChanged();
	void playbackStateChanged();
	void loopStateChanged();
	void loopSupportedChanged();
	void rateChanged();
	void minRateChanged();
	void maxRateChanged();
	void shuffleChanged();
	void shuffleSupportedChanged();
	void fullscreenChanged();
	void supportedUriSchemesChanged();
	void supportedMimeTypesChanged();

private slots:
	void onGetAllFinished();
	void onPositionChanged();
	void onExportedPositionChanged();
	void onSeek(qlonglong time);
	void onMetadataChanged();
	void onPlaybackStatusChanged();
	void onLoopStatusChanged();

private:
	// clang-format off
	dbus::DBusPropertyGroup appProperties;
	dbus::DBusProperty<QString> pIdentity {this->appProperties, "Identity"};
	dbus::DBusProperty<QString> pDesktopEntry {this->appProperties, "DesktopEntry", "", false};
	dbus::DBusProperty<bool> pCanQuit {this->appProperties, "CanQuit"};
	dbus::DBusProperty<bool> pCanRaise {this->appProperties, "CanRaise"};
	dbus::DBusProperty<bool> pFullscreen {this->appProperties, "Fullscreen", false, false};
	dbus::DBusProperty<bool> pCanSetFullscreen {this->appProperties, "CanSetFullscreen", false, false};
	dbus::DBusProperty<QList<QString>> pSupportedUriSchemes {this->appProperties, "SupportedUriSchemes"};
	dbus::DBusProperty<QList<QString>> pSupportedMimeTypes {this->appProperties, "SupportedMimeTypes"};

	dbus::DBusPropertyGroup playerProperties;
	dbus::DBusProperty<bool> pCanControl {this->playerProperties, "CanControl"};
	dbus::DBusProperty<bool> pCanPlay {this->playerProperties, "CanPlay"};
	dbus::DBusProperty<bool> pCanPause {this->playerProperties, "CanPause"};
	dbus::DBusProperty<bool> pCanSeek {this->playerProperties, "CanSeek"};
	dbus::DBusProperty<bool> pCanGoNext {this->playerProperties, "CanGoNext"};
	dbus::DBusProperty<bool> pCanGoPrevious {this->playerProperties, "CanGoPrevious"};
	dbus::DBusProperty<qlonglong> pPosition {this->playerProperties, "Position", 0, false}; // "required"
	dbus::DBusProperty<double> pVolume {this->playerProperties, "Volume", 1, false}; // "required"
	dbus::DBusProperty<QVariantMap> pMetadata {this->playerProperties, "Metadata"};
	dbus::DBusProperty<QString> pPlaybackStatus {this->playerProperties, "PlaybackStatus"};
	dbus::DBusProperty<QString> pLoopStatus {this->playerProperties, "LoopStatus", "", false};
	dbus::DBusProperty<double> pRate {this->playerProperties, "Rate", 1, false}; // "required"
	dbus::DBusProperty<double> pMinRate {this->playerProperties, "MinimumRate", 1, false}; // "required"
	dbus::DBusProperty<double> pMaxRate {this->playerProperties, "MaximumRate", 1, false}; // "required"
	dbus::DBusProperty<bool> pShuffle {this->playerProperties, "Shuffle", false, false};
	// clang-format on

	MprisPlaybackState::Enum mPlaybackState = MprisPlaybackState::Stopped;
	MprisLoopState::Enum mLoopState = MprisLoopState::None;
	QDateTime lastPositionTimestamp;
	QDateTime pausedTime;
	qlonglong mLength = -1;

	DBusMprisPlayerApp* app = nullptr;
	DBusMprisPlayer* player = nullptr;
	QString mTrackId;
	QString mUrl;
};

} // namespace qs::service::mpris
