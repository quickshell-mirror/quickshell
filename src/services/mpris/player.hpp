#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/doc.hpp"
#include "../../core/util.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_player.h"
#include "dbus_player_app.h"

namespace qs::service::mpris {

///! Playback state of an MprisPlayer
/// See @@MprisPlayer.playbackState.
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

	Q_INVOKABLE static QString toString(qs::service::mpris::MprisPlaybackState::Enum status);
};

///! Loop state of an MprisPlayer
/// See @@MprisPlayer.loopState.
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

	Q_INVOKABLE static QString toString(qs::service::mpris::MprisLoopState::Enum status);
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
	Q_PROPERTY(bool canControl READ canControl NOTIFY canControlChanged BINDABLE bindableCanControl);
	Q_PROPERTY(bool canPlay READ canPlay NOTIFY canPlayChanged BINDABLE bindableCanPlay);
	Q_PROPERTY(bool canPause READ canPause NOTIFY canPauseChanged BINDABLE bindableCanPause);
	Q_PROPERTY(bool canTogglePlaying READ canTogglePlaying NOTIFY canTogglePlayingChanged BINDABLE bindableCanTogglePlaying);
	Q_PROPERTY(bool canSeek READ canSeek NOTIFY canSeekChanged BINDABLE bindableCanSeek);
	Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY canGoNextChanged BINDABLE bindableCanGoNext);
	Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY canGoPreviousChanged BINDABLE bindableCanGoPrevious);
	Q_PROPERTY(bool canQuit READ canQuit NOTIFY canQuitChanged BINDABLE bindableCanQuit BINDABLE bindableCanQuit);
	Q_PROPERTY(bool canRaise READ canRaise NOTIFY canRaiseChanged BINDABLE bindableCanRaise BINDABLE bindableCanRaise);
	Q_PROPERTY(bool canSetFullscreen READ canSetFullscreen NOTIFY canSetFullscreenChanged BINDABLE bindableCanSetFullscreen);
	/// The human readable name of the media player.
	Q_PROPERTY(QString identity READ identity NOTIFY identityChanged BINDABLE bindableIdentity);
	/// The name of the desktop entry for the media player, or an empty string if not provided.
	Q_PROPERTY(QString desktopEntry READ desktopEntry NOTIFY desktopEntryChanged BINDABLE bindableDesktopEntry);
	/// The current position in the playing track, as seconds, with millisecond precision,
	/// or `0` if @@positionSupported is false.
	///
	/// May only be written to if @@canSeek and @@positionSupported are true.
	///
	/// > [!WARNING] To avoid excessive property updates wasting CPU while `position` is not
	/// > actively monitored, `position` usually will not update reactively, unless a nonlinear
	/// > change in position occurs, however reading it will always return the current position.
	/// >
	/// > If you want to actively monitor the position, the simplest way it to emit the @@positionChanged(s)
	/// > signal manually for the duration you are monitoring it, Using a @@QtQuick.FrameAnimation if you need
	/// > the value to update smoothly, such as on a slider, or a @@QtQuick.Timer if not, as shown below.
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
	Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged);
	Q_PROPERTY(bool positionSupported READ positionSupported NOTIFY positionSupportedChanged);
	/// The length of the playing track, as seconds, with millisecond precision,
	/// or the value of @@position if @@lengthSupported is false.
	Q_PROPERTY(qreal length READ length NOTIFY lengthChanged);
	Q_PROPERTY(bool lengthSupported READ lengthSupported NOTIFY lengthSupportedChanged);
	/// The volume of the playing track from 0.0 to 1.0, or 1.0 if @@volumeSupported is false.
	///
	/// May only be written to if @@canControl and @@volumeSupported are true.
	Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged BINDABLE bindableVolume);
	Q_PROPERTY(bool volumeSupported READ volumeSupported NOTIFY volumeSupportedChanged);
	/// Metadata of the current track.
	///
	/// A map of common properties is available [here](https://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata).
	/// Do not count on any of them actually being present.
	///
	/// Note that the @@trackTitle, @@trackAlbum, @@trackAlbumArtist, @@trackArtist and @@trackArtUrl
	/// properties have extra logic to guard against bad players sending weird metadata, and should
	/// be used over grabbing the properties directly from the metadata.
	Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged BINDABLE bindableMetadata);
	/// An opaque identifier for the current track unique within the current player.
	///
	/// > [!WARNING] This is NOT `mpris:trackid` as that is sometimes missing or nonunique
	/// > in some players.
	Q_PROPERTY(quint32 uniqueId READ uniqueId NOTIFY uniqueIdChanged BINDABLE bindableUniqueId);
	/// The title of the current track, or `""` if none was provided.
	///
	/// > [!TIP] Use `player.trackTitle || "Unknown Title"` to provide a message
	/// > when no title is available.
	Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY trackTitleChanged BINDABLE bindableTrackTitle);
	/// The current track's artist, or an `""` if none was provided.
	///
	/// > [!TIP] Use `player.trackArtist || "Unknown Artist"` to provide a message
	/// > when no artist is available.
	Q_PROPERTY(QString trackArtist READ trackArtist NOTIFY trackArtistChanged BINDABLE bindableTrackArtist);
	/// > [!ERROR] deprecated in favor of @@trackArtist.
	Q_PROPERTY(QString trackArtists READ trackArtist NOTIFY trackArtistChanged BINDABLE bindableTrackArtist);
	/// The current track's album, or `""` if none was provided.
	///
	/// > [!TIP] Use `player.trackAlbum || "Unknown Album"` to provide a message
	/// > when no album is available.
	Q_PROPERTY(QString trackAlbum READ trackAlbum NOTIFY trackAlbumChanged BINDABLE bindableTrackAlbum);
	/// The current track's album artist, or `""` if none was provided.
	///
	/// > [!TIP] Use `player.trackAlbumArtist || "Unknown Album"` to provide a message
	/// > when no album artist is available.
	Q_PROPERTY(QString trackAlbumArtist READ trackAlbumArtist NOTIFY trackAlbumArtistChanged BINDABLE bindableTrackAlbumArtist);
	/// The current track's art url, or `""` if none was provided.
	Q_PROPERTY(QString trackArtUrl READ trackArtUrl NOTIFY trackArtUrlChanged BINDABLE bindableTrackArtUrl);
	/// The playback state of the media player.
	///
	/// - If @@canPlay is false, you cannot assign the `Playing` state.
	/// - If @@canPause is false, you cannot assign the `Paused` state.
	/// - If @@canControl is false, you cannot assign the `Stopped` state.
	/// (or any of the others, though their repsective properties will also be false)
	Q_PROPERTY(qs::service::mpris::MprisPlaybackState::Enum playbackState READ playbackState WRITE setPlaybackState NOTIFY playbackStateChanged BINDABLE bindablePlaybackState);
	/// True if @@playbackState == `MprisPlaybackState.Playing`.
	///
	/// Setting this property is equivalent to calling @@play() or @@pause().
	/// You cannot set this property if @@canTogglePlaying is false.
	Q_PROPERTY(bool isPlaying READ isPlaying WRITE setPlaying NOTIFY isPlayingChanged BINDABLE bindableIsPlaying);
	/// The loop state of the media player, or `None` if @@loopSupported is false.
	///
	/// May only be written to if @@canControl and @@loopSupported are true.
	Q_PROPERTY(qs::service::mpris::MprisLoopState::Enum loopState READ loopState WRITE setLoopState NOTIFY loopStateChanged BINDABLE bindableLoopState);
	Q_PROPERTY(bool loopSupported READ loopSupported NOTIFY loopSupportedChanged);
	/// The speed the song is playing at, as a multiplier.
	///
	/// Only values between @@minRate and @@maxRate (inclusive) may be written to the property.
	/// Additionally, It is recommended that you only write common values such as `0.25`, `0.5`, `1.0`, `2.0`
	/// to the property, as media players are free to ignore the value, and are more likely to
	/// accept common ones.
	Q_PROPERTY(qreal rate READ rate WRITE setRate NOTIFY rateChanged BINDABLE bindableRate);
	Q_PROPERTY(qreal minRate READ minRate NOTIFY minRateChanged BINDABLE bindableMinRate);
	Q_PROPERTY(qreal maxRate READ maxRate NOTIFY maxRateChanged BINDABLE bindableMaxRate);
	/// If the play queue is currently being shuffled, or false if @@shuffleSupported is false.
	///
	/// May only be written if @@canControl and @@shuffleSupported are true.
	Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged BINDABLE bindableShuffle);
	Q_PROPERTY(bool shuffleSupported READ shuffleSupported NOTIFY shuffleSupportedChanged);
	/// If the player is currently shown in fullscreen.
	///
	/// May only be written to if @@canSetFullscreen is true.
	Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged BINDABLE bindableFullscreen);
	/// Uri schemes supported by @@openUri().
	Q_PROPERTY(QList<QString> supportedUriSchemes READ supportedUriSchemes NOTIFY supportedUriSchemesChanged BINDABLE bindableSupportedUriSchemes);
	/// Mime types supported by @@openUri().
	Q_PROPERTY(QList<QString> supportedMimeTypes READ supportedMimeTypes NOTIFY supportedMimeTypesChanged BINDABLE bindableSupportedMimeTypes);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("MprisPlayers can only be acquired from Mpris");

public:
	explicit MprisPlayer(const QString& address, QObject* parent = nullptr);

	/// Bring the media player to the front of the window stack.
	///
	/// May only be called if @@canRaise is true.
	Q_INVOKABLE void raise();
	/// Quit the media player.
	///
	/// May only be called if @@canQuit is true.
	Q_INVOKABLE void quit();
	/// Open the given URI in the media player.
	///
	/// Many players will silently ignore this, especially if the uri
	/// does not match @@supportedUriSchemes and @@supportedMimeTypes.
	Q_INVOKABLE void openUri(const QString& uri);
	/// Play the next song.
	///
	/// May only be called if @@canGoNext is true.
	Q_INVOKABLE void next();
	/// Play the previous song, or go back to the beginning of the current one.
	///
	/// May only be called if @@canGoPrevious is true.
	Q_INVOKABLE void previous();
	/// Change `position` by an offset.
	///
	/// Even if @@positionSupported is false and you cannot set `position`,
	/// this function may work.
	///
	/// May only be called if @@canSeek is true.
	Q_INVOKABLE void seek(qreal offset);
	/// Equivalent to setting @@playbackState to `Playing`.
	Q_INVOKABLE void play();
	/// Equivalent to setting @@playbackState to `Paused`.
	Q_INVOKABLE void pause();
	/// Equivalent to setting @@playbackState to `Stopped`.
	Q_INVOKABLE void stop();
	/// Equivalent to calling @@play() if not playing or @@pause() if playing.
	///
	/// May only be called if @@canTogglePlaying is true, which is equivalent to
	/// @@canPlay or @@canPause() depending on the current playback state.
	Q_INVOKABLE void togglePlaying();

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString address() const;

	QS_BINDABLE_GETTER(bool, bCanControl, canControl, bindableCanControl);
	QS_BINDABLE_GETTER(bool, bCanSeek, canSeek, bindableCanSeek);
	QS_BINDABLE_GETTER(bool, bCanGoNext, canGoNext, bindableCanGoNext);
	QS_BINDABLE_GETTER(bool, bCanGoPrevious, canGoPrevious, bindableCanGoPrevious);
	QS_BINDABLE_GETTER(bool, bCanPlay, canPlay, bindableCanPlay);
	QS_BINDABLE_GETTER(bool, bCanPause, canPause, bindableCanPause);
	QS_BINDABLE_GETTER(bool, bCanTogglePlaying, canTogglePlaying, bindableCanTogglePlaying);
	QS_BINDABLE_GETTER(bool, bCanQuit, canQuit, bindableCanQuit);
	QS_BINDABLE_GETTER(bool, bCanRaise, canRaise, bindableCanRaise);
	QS_BINDABLE_GETTER(bool, bCanSetFullscreen, canSetFullscreen, bindableCanSetFullscreen);

	QS_BINDABLE_GETTER(QString, bIdentity, identity, bindableIdentity);
	QS_BINDABLE_GETTER(QString, bDesktopEntry, desktopEntry, bindableDesktopEntry);

	[[nodiscard]] qlonglong positionMs() const;
	[[nodiscard]] qreal position() const;
	[[nodiscard]] bool positionSupported() const;
	void setPosition(qreal position);

	[[nodiscard]] qreal length() const;
	[[nodiscard]] bool lengthSupported() const;

	QS_BINDABLE_GETTER(qreal, bVolume, volume, bindableVolume);
	[[nodiscard]] bool volumeSupported() const;
	void setVolume(qreal volume);

	QS_BINDABLE_GETTER(quint32, bUniqueId, uniqueId, bindableUniqueId);
	QS_BINDABLE_GETTER(QVariantMap, bMetadata, metadata, bindableMetadata);
	QS_BINDABLE_GETTER(QString, bTrackTitle, trackTitle, bindableTrackTitle);
	QS_BINDABLE_GETTER(QString, bTrackAlbum, trackAlbum, bindableTrackAlbum);
	QS_BINDABLE_GETTER(QString, bTrackAlbumArtist, trackAlbumArtist, bindableTrackAlbumArtist);
	QS_BINDABLE_GETTER(QString, bTrackArtist, trackArtist, bindableTrackArtist);
	QS_BINDABLE_GETTER(QString, bTrackArtUrl, trackArtUrl, bindableTrackArtUrl);

	QS_BINDABLE_GETTER(
	    MprisPlaybackState::Enum,
	    bPlaybackState,
	    playbackState,
	    bindablePlaybackState
	);

	void setPlaybackState(MprisPlaybackState::Enum playbackState);

	QS_BINDABLE_GETTER(bool, bIsPlaying, isPlaying, bindableIsPlaying);
	void setPlaying(bool playing);

	QS_BINDABLE_GETTER(MprisLoopState::Enum, bLoopState, loopState, bindableLoopState);
	[[nodiscard]] bool loopSupported() const;
	void setLoopState(MprisLoopState::Enum loopState);

	QS_BINDABLE_GETTER(qreal, bRate, rate, bindableRate);
	QS_BINDABLE_GETTER(qreal, bRate, minRate, bindableMinRate);
	QS_BINDABLE_GETTER(qreal, bRate, maxRate, bindableMaxRate);
	void setRate(qreal rate);

	QS_BINDABLE_GETTER(bool, bShuffle, shuffle, bindableShuffle);
	[[nodiscard]] bool shuffleSupported() const;
	void setShuffle(bool shuffle);

	QS_BINDABLE_GETTER(bool, bFullscreen, fullscreen, bindableFullscreen);
	void setFullscreen(bool fullscreen);

	QS_BINDABLE_GETTER(
	    QList<QString>,
	    bSupportedUriSchemes,
	    supportedUriSchemes,
	    bindableSupportedUriSchemes
	);

	QS_BINDABLE_GETTER(
	    QList<QString>,
	    bSupportedMimeTypes,
	    supportedMimeTypes,
	    bindableSupportedMimeTypes
	);

signals:
	/// The track has changed.
	///
	/// All track information properties that were sent by the player
	/// will be updated immediately following this signal. @@postTrackChanged
	/// will be sent after they update.
	///
	/// Track information properties: @@uniqueId, @@metadata, @@trackTitle,
	/// @@trackArtist, @@trackAlbum, @@trackAlbumArtist, @@trackArtUrl
	///
	/// > [!WARNING] Some particularly poorly behaved players will update metadata
	/// > *before* indicating the track has changed.
	void trackChanged();
	/// Sent after track info related properties have been updated, following @@trackChanged.
	///
	/// > [!WARNING] It is not safe to assume all track information is up to date after
	/// > this signal is emitted. A large number of players will update track information,
	/// > particularly @@trackArtUrl, slightly after this signal.
	void postTrackChanged();

	QSDOC_HIDE void ready();
	void canControlChanged();
	void canPlayChanged();
	void canPauseChanged();
	void canTogglePlayingChanged();
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
	void uniqueIdChanged();
	void trackTitleChanged();
	void trackArtistChanged();
	void trackAlbumChanged();
	void trackAlbumArtistChanged();
	void trackArtUrlChanged();
	void playbackStateChanged();
	void isPlayingChanged();
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
	void onExportedPositionChanged();
	void onSeek(qlonglong time);

private:
	void onMetadataChanged();
	void onPositionUpdated();
	void requestPositionUpdate() { this->pPosition.requestUpdate(); };

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bIdentity, &MprisPlayer::identityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bDesktopEntry, &MprisPlayer::desktopEntryChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanQuit, &MprisPlayer::canQuitChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanRaise, &MprisPlayer::canRaiseChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bFullscreen, &MprisPlayer::fullscreenChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanSetFullscreen, &MprisPlayer::canSetFullscreenChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QList<QString>, bSupportedUriSchemes, &MprisPlayer::supportedUriSchemesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QList<QString>, bSupportedMimeTypes, &MprisPlayer::supportedMimeTypesChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(MprisPlayer, appProperties);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pIdentity, bIdentity, appProperties, "Identity");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pDesktopEntry, bDesktopEntry, appProperties, "DesktopEntry");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanQuit, bCanQuit, appProperties, "CanQuit");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanRaise, bCanRaise, appProperties, "CanRaise");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pFullscreen, bFullscreen, appProperties, "Fullscreen");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanSetFullscreen, bCanSetFullscreen, appProperties, "CanSetFullscreen");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pSupportedUriSchemes, bSupportedUriSchemes, appProperties, "SupportedUriSchemes");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pSupportedMimeTypes, bSupportedMimeTypes, appProperties, "SupportedMimeTypes");

	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bpCanPlay);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bpCanPause);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bpCanSeek);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bpCanGoNext);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bpCanGoPrevious);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QVariantMap, bpMetadata);
	QS_BINDING_SUBSCRIBE_METHOD(MprisPlayer, bpMetadata, onMetadataChanged, onValueChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MprisPlayer, qlonglong, bpPosition, -1, &MprisPlayer::positionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bpPlaybackStatus);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bpLoopStatus);

	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanControl, &MprisPlayer::canControlChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanPlay, &MprisPlayer::canPlayChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanPause, &MprisPlayer::canPauseChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanTogglePlaying, &MprisPlayer::canTogglePlayingChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanSeek, &MprisPlayer::canSeekChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanGoNext, &MprisPlayer::canGoNextChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bCanGoPrevious, &MprisPlayer::canGoPreviousChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MprisPlayer, qreal, bVolume, 1, &MprisPlayer::volumeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, MprisPlaybackState::Enum, bPlaybackState, &MprisPlayer::playbackStateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bIsPlaying, &MprisPlayer::isPlayingChanged);
	QS_BINDING_SUBSCRIBE_METHOD(MprisPlayer, bPlaybackState, requestPositionUpdate, onValueChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, MprisLoopState::Enum, bLoopState, &MprisPlayer::loopStateChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MprisPlayer, qreal, bRate, 1, &MprisPlayer::rateChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MprisPlayer, qreal, bMinRate, 1, &MprisPlayer::minRateChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MprisPlayer, qreal, bMaxRate, 1, &MprisPlayer::maxRateChanged);

	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QVariantMap, bMetadata, &MprisPlayer::metadataChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, quint32, bUniqueId, &MprisPlayer::uniqueIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bTrackTitle, &MprisPlayer::trackTitleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bTrackArtist, &MprisPlayer::trackArtistChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bTrackAlbum, &MprisPlayer::trackAlbumChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bTrackAlbumArtist, &MprisPlayer::trackAlbumArtistChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, QString, bTrackArtUrl, &MprisPlayer::trackArtUrlChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, qlonglong, bInternalLength, &MprisPlayer::lengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MprisPlayer, bool, bShuffle, &MprisPlayer::shuffleChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(MprisPlayer, playerProperties);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanControl, bCanControl, playerProperties, "CanControl");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanPlay, bpCanPlay, playerProperties, "CanPlay");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanPause, bpCanPause, playerProperties, "CanPause");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanSeek, bpCanSeek, playerProperties, "CanSeek");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanGoNext, bpCanGoNext, playerProperties, "CanGoNext");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pCanGoPrevious, bpCanGoPrevious, playerProperties, "CanGoPrevious");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, qlonglong, pPosition, bpPosition, onPositionUpdated, playerProperties, "Position", false);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pVolume, bVolume, playerProperties, "Volume", false);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pMetadata, bpMetadata, playerProperties, "Metadata");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pPlaybackStatus, bpPlaybackStatus, playerProperties, "PlaybackStatus");
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pLoopStatus, bpLoopStatus, playerProperties, "LoopStatus", false);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pRate, bRate, playerProperties, "Rate", false);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pMinRate, bMinRate, playerProperties, "MinimumRate", false);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pMaxRate, bMaxRate, playerProperties, "MaximumRate", false);
	QS_DBUS_PROPERTY_BINDING(MprisPlayer, pShuffle, bShuffle, playerProperties, "Shuffle", false);
	// clang-format on

	QDateTime lastPositionTimestamp;
	QDateTime pausedTime;

	DBusMprisPlayerApp* app = nullptr;
	DBusMprisPlayer* player = nullptr;
	QString mTrackId;
	QString mTrackUrl;
};

} // namespace qs::service::mpris
