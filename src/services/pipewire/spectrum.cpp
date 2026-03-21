#include "spectrum.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <numbers>
#include <numeric>
#include <vector>

#include <pipewire/core.h>
#include <pipewire/keys.h>
#include <pipewire/properties.h>
#include <pipewire/stream.h>
#include <qbytearray.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qscopeguard.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/param/audio/format.h>
#include <spa/param/audio/raw-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/param/format-utils.h>
#include <spa/param/format.h>
#include <spa/param/param.h>
#include <spa/pod/pod.h>

#include "../../core/logcat.hpp"
#include "connection.hpp"
#include "core.hpp"
#include "node.hpp"
#include "qml.hpp"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wmissing-designated-field-initializers"
#else
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace qs::service::pipewire {

namespace {

QS_LOGGING_CATEGORY(logSpectrum, "quickshell.service.pipewire.spectrum", QtWarningMsg);

// In-place radix-2 Cooley-Tukey FFT for complex data of length n (must be power of 2).
void fft(std::complex<float>* data, int n) {
	// Bit-reversal permutation
	for (int i = 1, j = 0; i < n; i++) {
		int bit = n >> 1;
		for (; j & bit; bit >>= 1) {
			j ^= bit;
		}
		j ^= bit;
		if (i < j) std::swap(data[i], data[j]);
	}

	// Butterfly stages
	for (int len = 2; len <= n; len <<= 1) {
		auto angle = -2.0f * std::numbers::pi_v<float> / static_cast<float>(len);
		std::complex<float> wn(std::cos(angle), std::sin(angle));
		for (int i = 0; i < n; i += len) {
			std::complex<float> w(1.0f, 0.0f);
			int half = len / 2;
			for (int j = 0; j < half; j++) {
				auto u = data[i + j];
				auto v = data[i + j + half] * w;
				data[i + j] = u + v;
				data[i + j + half] = u - v;
				w *= wn;
			}
		}
	}
}

} // anonymous namespace

// --- PwSpectrumStream: manages the Pipewire capture stream ---

class PwSpectrumStream {
public:
	PwSpectrumStream(PwAudioSpectrum* spectrum, PwNode* node)
	    : spectrum(spectrum), node(node) {}

	~PwSpectrumStream() { this->destroy(); }
	Q_DISABLE_COPY_MOVE(PwSpectrumStream);

	bool start();
	void destroy();

private:
	static const pw_stream_events EVENTS;
	static void onProcess(void* data);
	static void onParamChanged(void* data, uint32_t id, const spa_pod* param);
	static void
	onStateChanged(void* data, pw_stream_state oldState, pw_stream_state state, const char* error);
	static void onDestroy(void* data);

	void handleProcess();
	void handleParamChanged(uint32_t id, const spa_pod* param);

	PwAudioSpectrum* spectrum = nullptr;
	PwNode* node = nullptr;
	pw_stream* stream = nullptr;
	SpaHook listener;
	spa_audio_info_raw format = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_UNKNOWN);
	bool formatReady = false;
};

const pw_stream_events PwSpectrumStream::EVENTS = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = &PwSpectrumStream::onDestroy,
    .state_changed = &PwSpectrumStream::onStateChanged,
    .param_changed = &PwSpectrumStream::onParamChanged,
    .process = &PwSpectrumStream::onProcess,
};

bool PwSpectrumStream::start() {
	auto* core = PwConnection::instance()->registry.core;
	if (core == nullptr || !core->isValid()) {
		qCWarning(logSpectrum) << "Cannot start spectrum stream: pipewire core not ready.";
		return false;
	}

	auto target =
	    QByteArray::number(this->node->objectSerial ? this->node->objectSerial : this->node->id);

	// clang-format off
	auto* props = pw_properties_new(
	    PW_KEY_MEDIA_TYPE, "Audio",
	    PW_KEY_MEDIA_CATEGORY, "Monitor",
	    PW_KEY_MEDIA_NAME, "Spectrum analyzer",
	    PW_KEY_APP_NAME, "Quickshell Spectrum",
	    PW_KEY_STREAM_MONITOR, "true",
	    PW_KEY_STREAM_CAPTURE_SINK, this->node->type.testFlags(PwNodeType::Sink) ? "true" : "false",
	    PW_KEY_TARGET_OBJECT, target.constData(),
	    PW_KEY_NODE_PASSIVE, "true",
	    nullptr
	);
	// clang-format on

	if (props == nullptr) {
		qCWarning(logSpectrum) << "Failed to create properties for spectrum stream.";
		return false;
	}

	this->stream = pw_stream_new(core->core, "quickshell-spectrum", props);
	if (this->stream == nullptr) {
		pw_properties_free(props);
		qCWarning(logSpectrum) << "Failed to create spectrum stream.";
		return false;
	}

	pw_stream_add_listener(this->stream, &this->listener.hook, &PwSpectrumStream::EVENTS, this);

	auto buffer = std::array<quint8, 512> {};
	auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size()); // NOLINT

	auto params = std::array<const spa_pod*, 1> {};
	auto raw = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32);
	params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &raw);

	auto flags =
	    static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);
	auto res =
	    pw_stream_connect(this->stream, PW_DIRECTION_INPUT, PW_ID_ANY, flags, params.data(), 1);

	if (res < 0) {
		qCWarning(logSpectrum) << "Failed to connect spectrum stream:" << res;
		this->destroy();
		return false;
	}

	return true;
}

void PwSpectrumStream::destroy() {
	if (this->stream == nullptr) return;
	this->listener.remove();
	pw_stream_destroy(this->stream);
	this->stream = nullptr;
	this->formatReady = false;
}

void PwSpectrumStream::onProcess(void* data) {
	static_cast<PwSpectrumStream*>(data)->handleProcess(); // NOLINT
}

void PwSpectrumStream::onParamChanged(void* data, uint32_t id, const spa_pod* param) {
	static_cast<PwSpectrumStream*>(data)->handleParamChanged(id, param); // NOLINT
}

void PwSpectrumStream::onStateChanged(
    void* data,
    pw_stream_state oldState,
    pw_stream_state state,
    const char* error
) {
	Q_UNUSED(data);
	Q_UNUSED(oldState);
	if (state == PW_STREAM_STATE_ERROR) {
		if (error != nullptr) {
			qCWarning(logSpectrum) << "Spectrum stream error:" << error;
		} else {
			qCWarning(logSpectrum) << "Spectrum stream error.";
		}
	}
}

void PwSpectrumStream::onDestroy(void* data) {
	auto* self = static_cast<PwSpectrumStream*>(data); // NOLINT
	self->stream = nullptr;
	self->listener.remove();
	self->formatReady = false;
}

void PwSpectrumStream::handleParamChanged(uint32_t id, const spa_pod* param) {
	if (param == nullptr || id != SPA_PARAM_Format) return;

	auto info = spa_audio_info {};
	if (spa_format_parse(param, &info.media_type, &info.media_subtype) < 0) return;
	if (info.media_type != SPA_MEDIA_TYPE_audio || info.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		return;

	auto raw = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_UNKNOWN); // NOLINT
	if (spa_format_audio_raw_parse(param, &raw) < 0) return;

	if (raw.format != SPA_AUDIO_FORMAT_F32) {
		qCWarning(logSpectrum) << "Unsupported spectrum format:" << raw.format;
		this->formatReady = false;
		return;
	}

	this->format = raw;
	this->formatReady = raw.channels > 0;

	if (this->formatReady) {
		this->spectrum->mSampleRate = static_cast<int>(raw.rate);
		this->spectrum->computeBandBins();
		qCDebug(logSpectrum) << "Spectrum format:" << raw.rate << "Hz," << raw.channels << "ch";
	}
}

void PwSpectrumStream::handleProcess() {
	if (!this->formatReady || this->stream == nullptr) return;

	auto* buffer = pw_stream_dequeue_buffer(this->stream);
	if (buffer == nullptr) return;
	auto requeue = qScopeGuard([&, this] { pw_stream_queue_buffer(this->stream, buffer); });

	auto* spaBuffer = buffer->buffer;
	if (spaBuffer == nullptr || spaBuffer->n_datas < 1) return;

	auto* data = &spaBuffer->datas[0]; // NOLINT
	if (data->data == nullptr || data->chunk == nullptr) return;

	auto channelCount = static_cast<int>(this->format.channels);
	if (channelCount <= 0) return;

	const auto* base = static_cast<const quint8*>(data->data) + data->chunk->offset; // NOLINT
	const auto* samples = reinterpret_cast<const float*>(base);                       // NOLINT
	auto totalSamples = static_cast<int>(data->chunk->size / sizeof(float));
	auto frameCount = totalSamples / channelCount;

	if (frameCount <= 0) return;

	// Mix to mono using a static buffer to avoid per-callback allocation
	static thread_local std::vector<float> mono; // NOLINT
	mono.resize(frameCount);

	if (channelCount == 1) {
		std::copy(samples, samples + frameCount, mono.begin());
	} else {
		auto invChannels = 1.0f / static_cast<float>(channelCount);
		for (int i = 0; i < frameCount; i++) {
			float sum = 0.0f;
			for (int c = 0; c < channelCount; c++) {
				sum += samples[i * channelCount + c]; // NOLINT
			}
			mono[i] = sum * invChannels;
		}
	}

	this->spectrum->feedSamples(mono.data(), frameCount);
	this->spectrum->mSamplesReceived = true;
}

// --- PwAudioSpectrum ---

PwAudioSpectrum::PwAudioSpectrum(QObject* parent): QObject(parent) {
	this->initProcessing();
	QObject::connect(&this->mFrameTimer, &QTimer::timeout, this, &PwAudioSpectrum::onFrameTick);
}

PwAudioSpectrum::~PwAudioSpectrum() {
	delete this->mStream;
	this->mStream = nullptr;
}

PwNodeIface* PwAudioSpectrum::node() const { return this->mNode; }

void PwAudioSpectrum::setNode(PwNodeIface* node) {
	if (node == this->mNode) return;

	if (this->mNode != nullptr) {
		QObject::disconnect(this->mNode, nullptr, this, nullptr);
	}

	if (node != nullptr) {
		QObject::connect(node, &QObject::destroyed, this, &PwAudioSpectrum::onNodeDestroyed);
	}

	this->mNode = node;
	this->mNodeRef.setObject(node != nullptr ? node->node() : nullptr);
	qCDebug(logSpectrum) << "setNode:" << (node != nullptr ? "valid node" : "null");
	this->rebuildStream();
	emit this->nodeChanged();
}

bool PwAudioSpectrum::isEnabled() const { return this->mEnabled; }

void PwAudioSpectrum::setEnabled(bool enabled) {
	if (enabled == this->mEnabled) return;
	this->mEnabled = enabled;
	qCDebug(logSpectrum) << "setEnabled:" << enabled;
	this->rebuildStream();
	emit this->enabledChanged();
}

int PwAudioSpectrum::bandCount() const { return this->mBandCount; }

void PwAudioSpectrum::setBandCount(int count) {
	count = std::clamp(count, 1, FFT_SIZE / 2);
	if (count == this->mBandCount) return;
	this->mBandCount = count;
	this->initProcessing();
	emit this->bandCountChanged();
}

int PwAudioSpectrum::frameRate() const { return this->mFrameRate; }

void PwAudioSpectrum::setFrameRate(int rate) {
	rate = std::clamp(rate, 1, 240);
	if (rate == this->mFrameRate) return;
	this->mFrameRate = rate;
	if (this->mFrameTimer.isActive()) {
		this->mFrameTimer.setInterval(1000 / this->mFrameRate);
	}
	emit this->frameRateChanged();
}

int PwAudioSpectrum::lowerCutoff() const { return this->mLowerCutoff; }

void PwAudioSpectrum::setLowerCutoff(int freq) {
	freq = std::max(1, freq);
	if (freq == this->mLowerCutoff) return;
	this->mLowerCutoff = freq;
	this->computeBandBins();
	emit this->lowerCutoffChanged();
}

int PwAudioSpectrum::upperCutoff() const { return this->mUpperCutoff; }

void PwAudioSpectrum::setUpperCutoff(int freq) {
	freq = std::max(this->mLowerCutoff + 1, freq);
	if (freq == this->mUpperCutoff) return;
	this->mUpperCutoff = freq;
	this->computeBandBins();
	emit this->upperCutoffChanged();
}

qreal PwAudioSpectrum::noiseReduction() const { return this->mNoiseReduction; }

void PwAudioSpectrum::setNoiseReduction(qreal amount) {
	amount = std::clamp(amount, 0.0, 1.0);
	if (qFuzzyCompare(amount, this->mNoiseReduction)) return;
	this->mNoiseReduction = amount;
	this->mCachedGravityFrameRate = 0; // invalidate gravity cache
	emit this->noiseReductionChanged();
}

bool PwAudioSpectrum::smoothing() const { return this->mSmoothing; }

void PwAudioSpectrum::setSmoothing(bool enabled) {
	if (enabled == this->mSmoothing) return;
	this->mSmoothing = enabled;
	emit this->smoothingChanged();
}

void PwAudioSpectrum::onNodeDestroyed() {
	this->mNode = nullptr;
	this->mNodeRef.setObject(nullptr);
	this->rebuildStream();
	emit this->nodeChanged();
}

void PwAudioSpectrum::rebuildStream() {
	delete this->mStream;
	this->mStream = nullptr;

	auto* node = this->mNodeRef.object();
	qCDebug(logSpectrum) << "rebuildStream: enabled=" << this->mEnabled
	                     << "node=" << (node != nullptr)
	                     << "audioFlag=" << (node != nullptr ? node->type.testFlags(PwNodeType::Audio) : false);
	if (!this->mEnabled || node == nullptr || !node->type.testFlags(PwNodeType::Audio)) {
		this->mFrameTimer.stop();
		if (!this->mValues.isEmpty()) {
			this->mValues = QList<float>(this->mBandCount, 0.0f);
			emit this->valuesChanged();
		}
		if (!this->mIdle) {
			this->mIdle = true;
			emit this->idleChanged();
		}
		return;
	}

	qCDebug(logSpectrum) << "rebuildStream: creating stream for node id=" << node->id;
	this->mStream = new PwSpectrumStream(this, node);
	if (!this->mStream->start()) {
		delete this->mStream;
		this->mStream = nullptr;
		this->mFrameTimer.stop();
		if (!this->mValues.isEmpty()) {
			this->mValues = QList<float>(this->mBandCount, 0.0f);
			emit this->valuesChanged();
		}
		if (!this->mIdle) {
			this->mIdle = true;
			emit this->idleChanged();
		}
		return;
	}

	// Reset processing state
	this->mRingPos = 0;
	this->mRingFull = false;
	this->mIdleFrames = 0;
	this->mSamplesReceived = false;
	this->mSensitivity = 0.01f;
	this->mSensInit = true;
	std::fill(this->mPrevBands.begin(), this->mPrevBands.end(), 0.0f);
	std::fill(this->mPeak.begin(), this->mPeak.end(), 0.0f);
	std::fill(this->mFall.begin(), this->mFall.end(), 0.0f);
	std::fill(this->mMem.begin(), this->mMem.end(), 0.0f);

	this->mFrameTimer.setInterval(1000 / this->mFrameRate);
	this->mFrameTimer.start();
	qCDebug(logSpectrum) << "rebuildStream: stream started, timer at" << (1000 / this->mFrameRate) << "ms";
}

void PwAudioSpectrum::initProcessing() {
	this->mRingBuffer.resize(FFT_SIZE, 0.0f);
	this->mRingPos = 0;
	this->mRingFull = false;
	this->mFftBuf.resize(FFT_SIZE);

	// Pre-compute Hann window
	this->mWindow.resize(FFT_SIZE);
	for (int i = 0; i < FFT_SIZE; i++) {
		this->mWindow[i] = 0.5f
		    * (1.0f
		       - std::cos(
		           2.0f * std::numbers::pi_v<float> * static_cast<float>(i)
		           / static_cast<float>(FFT_SIZE - 1)
		       ));
	}

	this->mPrevBands.assign(this->mBandCount, 0.0f);
	this->mPeak.assign(this->mBandCount, 0.0f);
	this->mFall.assign(this->mBandCount, 0.0f);
	this->mMem.assign(this->mBandCount, 0.0f);
	this->mBands.assign(this->mBandCount, 0.0f);
	this->mValues = QList<float>(this->mBandCount, 0.0f);
	this->mCachedGravityFrameRate = 0; // force recompute
	this->computeBandBins();
}

void PwAudioSpectrum::computeBandBins() {
	this->mBandBinLow.resize(this->mBandCount);
	this->mBandBinHigh.resize(this->mBandCount);

	auto fLow = static_cast<float>(this->mLowerCutoff);
	auto fHigh =
	    static_cast<float>(std::min(this->mUpperCutoff, this->mSampleRate / 2));
	auto ratio = fHigh / fLow;
	auto fftBins = FFT_SIZE / 2;

	for (int i = 0; i < this->mBandCount; i++) {
		auto barFreqLow = fLow
		    * std::pow(ratio, static_cast<float>(i) / static_cast<float>(this->mBandCount));
		auto barFreqHigh = fLow
		    * std::pow(
		        ratio,
		        static_cast<float>(i + 1) / static_cast<float>(this->mBandCount)
		    );

		auto binLow = static_cast<int>(
		    std::ceil(barFreqLow * static_cast<float>(FFT_SIZE) / static_cast<float>(this->mSampleRate))
		);
		auto binHigh = static_cast<int>(
		    std::floor(barFreqHigh * static_cast<float>(FFT_SIZE) / static_cast<float>(this->mSampleRate))
		);

		binLow = std::clamp(binLow, 1, fftBins);
		binHigh = std::clamp(binHigh, binLow, fftBins);

		// Anti-clumping: ensure this band doesn't overlap with the previous band's bins.
		// When multiple bands map to the same FFT bin (common in bass with limited
		// frequency resolution), nudge this band's lower bound up so each band gets
		// at least one unique bin.
		if (i > 0 && binLow <= this->mBandBinHigh[i - 1]) {
			binLow = this->mBandBinHigh[i - 1] + 1;
			if (binLow > fftBins) binLow = fftBins;
			if (binHigh < binLow) binHigh = binLow;
		}

		this->mBandBinLow[i] = binLow;
		this->mBandBinHigh[i] = binHigh;
	}
}

void PwAudioSpectrum::feedSamples(const float* monoSamples, int count) {
	bool wasFull = this->mRingFull;
	for (int i = 0; i < count; i++) {
		this->mRingBuffer[this->mRingPos] = monoSamples[i]; // NOLINT
		this->mRingPos = (this->mRingPos + 1) % FFT_SIZE;
		if (this->mRingPos == 0) this->mRingFull = true;
	}
	if (!wasFull && this->mRingFull) {
		qCDebug(logSpectrum) << "Ring buffer full, FFT processing will begin";
	}
}

void PwAudioSpectrum::onFrameTick() { this->processFrame(); }

void PwAudioSpectrum::processFrame() {
	if (!this->mRingFull) return;

	// Already idle - skip all processing
	if (this->mIdle && !this->mSamplesReceived) return;

	// 0. If no new samples arrived since last frame, fade the ring buffer toward
	//    silence so stale data doesn't keep the bands stuck when the audio source closes.
	if (!this->mSamplesReceived) {
		for (auto& s: this->mRingBuffer) {
			s *= 0.85f;
		}
	}
	this->mSamplesReceived = false;

	// 1. Copy ring buffer to FFT buffer with Hann window applied
	for (int i = 0; i < FFT_SIZE; i++) {
		int idx = (this->mRingPos + i) % FFT_SIZE;
		this->mFftBuf[i] = {this->mRingBuffer[idx] * this->mWindow[i], 0.0f};
	}

	// 2. Compute FFT
	fft(this->mFftBuf.data(), FFT_SIZE);

	// 3. Map FFT bins to bands using logarithmic frequency distribution.
	//    For each band, take the peak magnitude squared across its frequency range,
	//    then sqrt once per band (avoids sqrt per bin).
	auto& bands = this->mBands;
	for (int i = 0; i < this->mBandCount; i++) {
		float maxMagSq = 0.0f;
		for (int bin = this->mBandBinLow[i]; bin <= this->mBandBinHigh[i]; bin++) {
			float magSq = std::norm(this->mFftBuf[bin]);
			if (magSq > maxMagSq) maxMagSq = magSq;
		}
		bands[i] = std::sqrt(maxMagSq);
	}

	// 4. Frequency weighting: perceptual boost for lower frequencies.
	//    Low-frequency bands cover fewer FFT bins and need compensation.
	auto invBandCount = 1.0f / static_cast<float>(this->mBandCount);
	for (int i = 0; i < this->mBandCount; i++) {
		float weight = 1.0f
		    + 0.5f * static_cast<float>(this->mBandCount - i) * invBandCount;
		bands[i] *= weight;
	}

	// 5. Noise gate: subtract a fixed threshold from raw magnitudes before
	//    auto-sensitivity can amplify noise. The threshold scales with FFT size
	//    so it stays proportional to the magnitude range.
	auto nrFactor = static_cast<float>(this->mNoiseReduction);
	float noiseGate = nrFactor * static_cast<float>(FFT_SIZE) * 0.00005f;
	for (auto& band: bands) {
		band = std::max(0.0f, band - noiseGate);
	}

	// 6. Auto-sensitivity: apply current sensitivity, then adjust conservatively.
	//    2% decrease on overshoot, 0.1% increase per frame.
	for (auto& band: bands) {
		band *= this->mSensitivity;
	}

	// 7. Gravity falloff + integral smoothing.
	//    Gravity: when a band drops, it falls from its stored peak with quadratic
	//    acceleration, giving a natural physics-based decay.
	//    Integral: IIR low-pass filter (memory * noiseReduction + current) damps
	//    rapid transients for a smoother, less jittery response.
	if (this->mCachedGravityFrameRate != this->mFrameRate) {
		auto fps = static_cast<double>(this->mFrameRate);
		this->mCachedGravityMod = std::pow(60.0 / fps, 2.5) * 1.54 / std::max(this->mNoiseReduction, 0.01);
		if (this->mCachedGravityMod < 1.0) this->mCachedGravityMod = 1.0;
		this->mCachedGravityFrameRate = this->mFrameRate;
	}
	auto gravityMod = this->mCachedGravityMod;

	bool overshoot = false;
	bool silence = true;
	for (int i = 0; i < this->mBandCount; i++) {
		// Gravity falloff
		if (bands[i] < this->mPrevBands[i] && this->mNoiseReduction > 0.1) {
			bands[i] = static_cast<float>(
			    static_cast<double>(this->mPeak[i])
			    * (1.0 - static_cast<double>(this->mFall[i]) * static_cast<double>(this->mFall[i]) * gravityMod)
			);
			if (bands[i] < 0.0f) bands[i] = 0.0f;
			this->mFall[i] += 0.028f;
		} else {
			this->mPeak[i] = bands[i];
			this->mFall[i] = 0.0f;
		}
		this->mPrevBands[i] = bands[i];

		// Integral smoothing
		bands[i] = this->mMem[i] * nrFactor + bands[i];
		this->mMem[i] = bands[i];

		if (bands[i] > 1.0f) {
			overshoot = true;
			bands[i] = 1.0f;
		}
		if (bands[i] > 0.01f) silence = false;
	}

	// Auto-sensitivity adjustment
	if (overshoot) {
		this->mSensitivity *= 0.98f;
		this->mSensInit = false;
	} else if (!silence) {
		this->mSensitivity *= 1.001f;
		if (this->mSensInit) this->mSensitivity *= 1.1f;
	}
	this->mSensitivity = std::clamp(this->mSensitivity, 0.001f, 50.0f);

	// 8. Monstercat-style spatial smoothing: each band propagates
	//    its value to neighbors with exponential distance decay.
	//    Uses iterative division instead of std::pow() for the decay factor.
	if (this->mSmoothing) {
		constexpr float MONSTERCAT_FACTOR = 1.5f;
		constexpr float MIN_SPREAD = 0.001f;
		for (int z = 0; z < this->mBandCount; z++) {
			float spread = bands[z] / MONSTERCAT_FACTOR;
			for (int m = z - 1; m >= 0 && spread > MIN_SPREAD; m--) {
				if (spread > bands[m]) bands[m] = spread;
				spread /= MONSTERCAT_FACTOR;
			}
			spread = bands[z] / MONSTERCAT_FACTOR;
			for (int m = z + 1; m < this->mBandCount && spread > MIN_SPREAD; m++) {
				if (spread > bands[m]) bands[m] = spread;
				spread /= MONSTERCAT_FACTOR;
			}
		}
	}

	// 9. Clamp to 0-1
	for (auto& band: bands) {
		band = std::clamp(band, 0.0f, 1.0f);
	}

	// 10. Idle detection: if all bands are near zero for several consecutive frames,
	//     stop emitting updates to save GPU rendering.
	if (silence) {
		this->mIdleFrames++;
		if (this->mIdleFrames >= IDLE_THRESHOLD) {
			if (!this->mIdle) {
				this->mIdle = true;
				this->mValues = QList<float>(this->mBandCount, 0.0f);
				emit this->valuesChanged();
				emit this->idleChanged();
			}
			return;
		}
	} else {
		this->mIdleFrames = 0;
		if (this->mIdle) {
			this->mIdle = false;
			emit this->idleChanged();
		}
	}

	// 11. Emit updated values to QML (in-place update, no allocation)
	bool changed = false;
	for (int i = 0; i < this->mBandCount; i++) {
		if (this->mValues[i] != bands[i]) {
			changed = true;
			this->mValues[i] = bands[i];
		}
	}

	if (changed) {
		emit this->valuesChanged();
	}
}

} // namespace qs::service::pipewire

#pragma GCC diagnostic pop
