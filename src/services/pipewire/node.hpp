#pragma once

#include <pipewire/core.h>
#include <pipewire/node.h>
#include <pipewire/type.h>
#include <qcontainerfwd.h>
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
	Q_INVOKABLE static QString toString(PwAudioChannel::Enum value);
};

enum class PwNodeType {
	Untracked,
	Audio,
};

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

private:
	void updateVolumeProps(const spa_pod* param);

	bool mMuted = false;
	QVector<PwAudioChannel::Enum> mChannels;
	QVector<float> mVolumes;
	QVector<float> mDeviceVolumes;
	QVector<float> waitingVolumes;
	PwNode* node;
};

constexpr const char TYPE_INTERFACE_Node[] = PW_TYPE_INTERFACE_Node;             // NOLINT
class PwNode: public PwBindable<pw_node, TYPE_INTERFACE_Node, PW_VERSION_NODE> { // NOLINT
	Q_OBJECT;

public:
	void bindHooks() override;
	void unbindHooks() override;
	void initProps(const spa_dict* props) override;

	QString name;
	QString description;
	QString nick;
	QMap<QString, QString> properties;

	PwNodeType type = PwNodeType::Untracked;
	bool isSink = false;
	bool isStream = false;

	PwNodeBoundData* boundData = nullptr;

	PwDevice* device = nullptr;
	qint32 routeDevice = -1;

signals:
	void propertiesChanged();

private:
	static const pw_node_events EVENTS;
	static void onInfo(void* data, const pw_node_info* info);
	static void
	onParam(void* data, qint32 seq, quint32 id, quint32 index, quint32 next, const spa_pod* param);

	SpaHook listener;
};

} // namespace qs::service::pipewire
