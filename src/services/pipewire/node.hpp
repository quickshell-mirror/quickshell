#pragma once

#include <pipewire/core.h>
#include <pipewire/node.h>
#include <pipewire/type.h>
#include <qcontainerfwd.h>
#include <qflags.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/param/audio/raw.h>
#include <spa/pod/pod.h>

#include "core.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

class PwDevice;

///! Audio channel of a pipewire node.
/// See @@PwNodeAudio.channels.
class PwAudioChannel: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		Unknown = SPA_AUDIO_CHANNEL_UNKNOWN,
		NA = SPA_AUDIO_CHANNEL_NA,
		Mono = SPA_AUDIO_CHANNEL_MONO,
		FrontCenter = SPA_AUDIO_CHANNEL_FC,
		FrontLeft = SPA_AUDIO_CHANNEL_FL,
		FrontRight = SPA_AUDIO_CHANNEL_FR,
		FrontLeftCenter = SPA_AUDIO_CHANNEL_FLC,
		FrontRightCenter = SPA_AUDIO_CHANNEL_FRC,
		FrontLeftWide = SPA_AUDIO_CHANNEL_FLW,
		FrontRightWide = SPA_AUDIO_CHANNEL_FRW,
		FrontCenterHigh = SPA_AUDIO_CHANNEL_FCH,
		FrontLeftHigh = SPA_AUDIO_CHANNEL_FLH,
		FrontRightHigh = SPA_AUDIO_CHANNEL_FRH,
		LowFrequencyEffects = SPA_AUDIO_CHANNEL_LFE,
		LowFrequencyEffects2 = SPA_AUDIO_CHANNEL_LFE2,
		LowFrequencyEffectsLeft = SPA_AUDIO_CHANNEL_LLFE,
		LowFrequencyEffectsRight = SPA_AUDIO_CHANNEL_RLFE,
		SideLeft = SPA_AUDIO_CHANNEL_SL,
		SideRight = SPA_AUDIO_CHANNEL_SR,
		RearCenter = SPA_AUDIO_CHANNEL_RC,
		RearLeft = SPA_AUDIO_CHANNEL_RL,
		RearRight = SPA_AUDIO_CHANNEL_RR,
		RearLeftCenter = SPA_AUDIO_CHANNEL_RLC,
		RearRightCenter = SPA_AUDIO_CHANNEL_RRC,
		TopCenter = SPA_AUDIO_CHANNEL_TC,
		TopFrontCenter = SPA_AUDIO_CHANNEL_TFC,
		TopFrontLeft = SPA_AUDIO_CHANNEL_TFL,
		TopFrontRight = SPA_AUDIO_CHANNEL_TFR,
		TopFrontLeftCenter = SPA_AUDIO_CHANNEL_TFLC,
		TopFrontRightCenter = SPA_AUDIO_CHANNEL_TFRC,
		TopSideLeft = SPA_AUDIO_CHANNEL_TSL,
		TopSideRight = SPA_AUDIO_CHANNEL_TSR,
		TopRearCenter = SPA_AUDIO_CHANNEL_TRC,
		TopRearLeft = SPA_AUDIO_CHANNEL_TRL,
		TopRearRight = SPA_AUDIO_CHANNEL_TRR,
		BottomCenter = SPA_AUDIO_CHANNEL_BC,
		BottomLeftCenter = SPA_AUDIO_CHANNEL_BLC,
		BottomRightCenter = SPA_AUDIO_CHANNEL_BRC,
		/// The start of the aux channel range.
		///
		/// Values between AuxRangeStart and AuxRangeEnd are valid.
		AuxRangeStart = SPA_AUDIO_CHANNEL_START_Aux,
		/// The end of the aux channel range.
		///
		/// Values between AuxRangeStart and AuxRangeEnd are valid.
		AuxRangeEnd = SPA_AUDIO_CHANNEL_LAST_Aux,
		/// The end of the custom channel range.
		///
		/// Values starting at CustomRangeStart are valid.
		CustomRangeStart = SPA_AUDIO_CHANNEL_START_Custom,
	};
	Q_ENUM(Enum);

	/// Print a human readable representation of the given channel,
	/// including aux and custom channel ranges.
	Q_INVOKABLE static QString toString(qs::service::pipewire::PwAudioChannel::Enum value);
};
///! The type of a pipewire node.
/// Use bitwise comparisons to filter for audio, video, sink, source or stream nodes
class PwNodeType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Flag : quint8 {
		// A Pipewire node which is not being managed.
		Untracked = 0b0,
		// This flag is set when this node is an Audio node.
		Audio = 0b1,
		// This flag is set when this node is an Video node.
		Video = 0b10,
		// This flag is set when this node is a stream node.
		Stream = 0b100,
		// This flag is set when this node is producing some form of data,
		// such as a microphone, screenshare or webcam.
		Source = 0b1000,
		// This flag is set when this node is receiving data.
		Sink = 0b10000,
		// A sink for audio samples, like an audio card.
		//
		// This is equivalent to the media class `Video/Source` and is
		// composed of the @@PwNodeType.Audio and @@PwNodeType.Sink flags.
		AudioSink = Audio | Sink,
		// A source of audio samples like a microphone.
		//
		// This is quivalent to the media class `Video/Sink` and is composed
		// of the @@PwNodeType.Audio and @@PwNodeType.Source flags.
		AudioSource = Audio | Source,
		// A node that is both a sink and a source.
		//
		// This is equivalent to the media class `Audio/Duplex` and is composed of the
		// @@PwNodeType.Audio, @@PwNodeType.Source and @@PwNodeType.Sink flags.
		AudioDuplex = Audio | Sink | Source,
		// A playback stream.
		//
		// This is equivalent to the media class `Stream/Output/Audio` and is composed
		// of the @@PwNodeType.Audio, @@PwNodeType.Sink and @@PwNodeType.Stream flags.
		AudioOutStream = Audio | Sink | Stream,
		// A capture stream.
		//
		// This is equivalent to the media class `Stream/Input/Audio` and is composed
		// of the @@PwNodeType.Audio, @@PwNodeType.Source and @@PwNodeType.Stream flags.
		AudioInStream = Audio | Source | Stream,
		// A producer of video, like a webcam or a screenshare.
		//
		// This is equivalent to the media class `Video/Source` and is composed
		// of the @@PwNodeType.Video and @@PwNodeType.Source flags.
		VideoSource = Video | Source,
		// A consumer of video, such as a program that is recieving a video stream.
		//
		// This is equivalent to the media class `Video/Sink` and is composed of the
		// @@PwNodeType.Video and @@PwNodeType.Sink flags.
		VideoSink = Video | Sink,
	};
	Q_ENUM(Flag);
	Q_DECLARE_FLAGS(Flags, Flag);
	Q_INVOKABLE static QString toString(qs::service::pipewire::PwNodeType::Flags type);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PwNodeType::Flags)

class PwNode;

struct PwVolumeProps {
	QVector<PwAudioChannel::Enum> channels;
	QVector<float> volumes;
	bool mute = false;

	static PwVolumeProps parseSpaPod(const spa_pod* param);
};

class PwNodeBoundData {
public:
	PwNodeBoundData() = default;
	virtual ~PwNodeBoundData() = default;
	Q_DISABLE_COPY_MOVE(PwNodeBoundData);

	virtual void onInfo(const pw_node_info* /*info*/) {}
	virtual void onSpaParam(quint32 /*id*/, quint32 /*index*/, const spa_pod* /*param*/) {}
	virtual void onUnbind() {}
};

class PwNodeBoundAudio
    : public QObject
    , public PwNodeBoundData {
	Q_OBJECT;

public:
	explicit PwNodeBoundAudio(PwNode* node);

	void onInfo(const pw_node_info* info) override;
	void onSpaParam(quint32 id, quint32 index, const spa_pod* param) override;
	void onUnbind() override;

	[[nodiscard]] bool isMuted() const;
	void setMuted(bool muted);

	[[nodiscard]] float averageVolume() const;
	void setAverageVolume(float volume);

	[[nodiscard]] QVector<PwAudioChannel::Enum> channels() const;

	[[nodiscard]] QVector<float> volumes() const;
	void setVolumes(const QVector<float>& volumes);

signals:
	void volumesChanged();
	void channelsChanged();
	void mutedChanged();

private slots:
	void onDeviceReady();
	void onDeviceVolumesChanged(qint32 routeDevice, const PwVolumeProps& props);

private:
	void updateVolumeProps(const PwVolumeProps& volumeProps);

	bool mMuted = false;
	QVector<PwAudioChannel::Enum> mChannels;
	QVector<float> mVolumes;
	QVector<float> mServerVolumes;
	QVector<float> mDeviceVolumes;
	QVector<float> waitingVolumes;
	PwNode* node;
};

class PwNode: public PwBindable<pw_node, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE> {
	Q_OBJECT;

public:
	void bindHooks() override;
	void unbindHooks() override;
	void initProps(const spa_dict* props) override;

	QString name;
	QString description;
	QString nick;
	QMap<QString, QString> properties;

	PwNodeType::Flags type = PwNodeType::Untracked;

	bool ready = false;

	PwNodeBoundData* boundData = nullptr;

	PwDevice* device = nullptr;
	qint32 routeDevice = -1;

signals:
	void propertiesChanged();
	void readyChanged();

private slots:
	void onCoreSync(quint32 id, qint32 seq);

private:
	static const pw_node_events EVENTS;
	static void onInfo(void* data, const pw_node_info* info);
	static void
	onParam(void* data, qint32 seq, quint32 id, quint32 index, quint32 next, const spa_pod* param);

	qint32 syncSeq = 0;
	SpaHook listener;
};

} // namespace qs::service::pipewire
