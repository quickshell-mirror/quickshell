#pragma once

#include <complex>
#include <vector>

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtimer.h>
#include <qtypes.h>

#include "node.hpp"

namespace qs::service::pipewire {

class PwNodeIface;
class PwSpectrumStream;


///! Computes audio frequency spectrum from a Pipewire node.
/// Captures audio from a node and computes a frequency spectrum,
/// producing band values suitable for audio visualization.
///
/// Typical usage: bind @@node to `Pipewire.defaultAudioSink` for system-wide visualization.
///
/// ```qml
/// PwAudioSpectrum {
///     node: Pipewire.defaultAudioSink
///     enabled: true
///     bandCount: 32
///     frameRate: 30
/// }
/// ```
class PwAudioSpectrum: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The node to capture audio from. Must be an audio node.
	/// Bind to `Pipewire.defaultAudioSink` for system-wide audio visualization.
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* node READ node WRITE setNode NOTIFY nodeChanged);
	/// If true, the spectrum analyzer is actively capturing and computing. Defaults to false.
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged);
	/// Number of frequency bands to produce. Defaults to 32.
	Q_PROPERTY(int bandCount READ bandCount WRITE setBandCount NOTIFY bandCountChanged);
	/// Deprecated: use bandCount instead.
	Q_PROPERTY(int barCount READ bandCount WRITE setBandCount NOTIFY bandCountChanged);
	/// Target output frame rate in Hz. Defaults to 30.
	Q_PROPERTY(int frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged);
	/// Lower frequency cutoff in Hz. Defaults to 50.
	Q_PROPERTY(int lowerCutoff READ lowerCutoff WRITE setLowerCutoff NOTIFY lowerCutoffChanged);
	/// Upper frequency cutoff in Hz. Defaults to 12000.
	Q_PROPERTY(int upperCutoff READ upperCutoff WRITE setUpperCutoff NOTIFY upperCutoffChanged);
	/// Noise reduction amount (0.0-1.0). Higher values suppress more background noise. Defaults to 0.77.
	Q_PROPERTY(qreal noiseReduction READ noiseReduction WRITE setNoiseReduction NOTIFY noiseReductionChanged);
	/// Enable smoothing between adjacent bands for a less jittery look. Defaults to true.
	Q_PROPERTY(bool smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged);
	/// Per-band spectrum values, normalized to 0.0-1.0. Length equals @@bandCount.
	Q_PROPERTY(QList<float> values READ values NOTIFY valuesChanged);
	/// True when audio is silent (all bands near zero) for several consecutive frames.
	Q_PROPERTY(bool idle READ isIdle NOTIFY idleChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit PwAudioSpectrum(QObject* parent = nullptr);
	~PwAudioSpectrum() override;
	Q_DISABLE_COPY_MOVE(PwAudioSpectrum);

	[[nodiscard]] PwNodeIface* node() const;
	void setNode(PwNodeIface* node);

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] int bandCount() const;
	void setBandCount(int count);

	[[nodiscard]] int frameRate() const;
	void setFrameRate(int rate);

	[[nodiscard]] int lowerCutoff() const;
	void setLowerCutoff(int freq);

	[[nodiscard]] int upperCutoff() const;
	void setUpperCutoff(int freq);

	[[nodiscard]] qreal noiseReduction() const;
	void setNoiseReduction(qreal amount);

	[[nodiscard]] bool smoothing() const;
	void setSmoothing(bool enabled);

	[[nodiscard]] QList<float> values() const { return this->mValues; }
	[[nodiscard]] bool isIdle() const { return this->mIdle; }

signals:
	void nodeChanged();
	void enabledChanged();
	void bandCountChanged();
	void frameRateChanged();
	void lowerCutoffChanged();
	void upperCutoffChanged();
	void noiseReductionChanged();
	void smoothingChanged();
	void valuesChanged();
	void idleChanged();

private slots:
	void onNodeDestroyed();
	void onFrameTick();

private:
	friend class PwSpectrumStream;

	void rebuildStream();
	void feedSamples(const float* monoSamples, int count);
	void processFrame();
	void initProcessing();
	void computeBandBins();

	// QML state
	PwNodeIface* mNode = nullptr;
	PwBindableRef<PwNode> mNodeRef;
	bool mEnabled = false;
	int mBandCount = 32;
	int mFrameRate = 30;
	int mLowerCutoff = 50;
	int mUpperCutoff = 12000;
	qreal mNoiseReduction = 0.77;
	bool mSmoothing = true;
	QList<float> mValues;
	bool mIdle = true;

	PwSpectrumStream* mStream = nullptr;
	QTimer mFrameTimer;

	// Processing state
	static constexpr int FFT_SIZE = 4096;
	static constexpr int IDLE_THRESHOLD = 30;

	std::vector<float> mRingBuffer;
	int mRingPos = 0;
	bool mRingFull = false;
	int mSampleRate = 48000;
	int mIdleFrames = 0;
	bool mSamplesReceived = false;

	std::vector<float> mWindow; // Hann window
	std::vector<int> mBandBinLow; // lower FFT bin per band
	std::vector<int> mBandBinHigh; // upper FFT bin per band
	std::vector<float> mPrevBands; // previous output for comparison
	std::vector<float> mPeak; // peak tracking for gravity falloff
	std::vector<float> mFall; // fall accumulator per band (gravity)
	std::vector<float> mMem; // integral filter memory per band
	std::vector<float> mBands; // reusable per-frame band buffer
	float mSensitivity = 1.0f;
	bool mSensInit = true;
	double mCachedGravityMod = 1.0;
	int mCachedGravityFrameRate = 0;

	std::vector<std::complex<float>> mFftBuf;
};

} // namespace qs::service::pipewire
