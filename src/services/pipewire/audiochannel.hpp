#pragma once

#include <qqmlintegration.h>
#include <qobject.h>
#include <spa/param/audio/raw.h>

namespace qs::service::pipewire {
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
}
