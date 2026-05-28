#include "peak.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <pipewire/core.h>
#include <pipewire/keys.h>
#include <pipewire/port.h>
#include <pipewire/properties.h>
#include <pipewire/stream.h>
#include <qbytearray.h>
#include <qcontainerfwd.h>
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
#pragma GCC diagnostic ignored "-Wmissing-designated-field-initializers"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logPeak, "quickshell.service.pipewire.peak", QtWarningMsg);
}

class PwPeakStream {
public:
	PwPeakStream(PwNodePeakMonitor* monitor, PwNode* node): monitor(monitor), node(node) {}
	~PwPeakStream() { this->destroy(); }
	Q_DISABLE_COPY_MOVE(PwPeakStream);

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
	void handleStateChanged(pw_stream_state oldState, pw_stream_state state, const char* error);
	void resetFormat();

	PwNodePeakMonitor* monitor = nullptr;
	PwNode* node = nullptr;
	pw_stream* stream = nullptr;
	SpaHook listener;
	spa_audio_info_raw format = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_UNKNOWN);
	bool formatReady = false;
	QVector<float> channelPeaks;
};

const pw_stream_events PwPeakStream::EVENTS = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = &PwPeakStream::onDestroy,
    .state_changed = &PwPeakStream::onStateChanged,
    .param_changed = &PwPeakStream::onParamChanged,
    .process = &PwPeakStream::onProcess,
};

bool PwPeakStream::start() {
	auto* core = PwConnection::instance()->registry.core;
	if (core == nullptr || !core->isValid()) {
		qCWarning(logPeak) << "Cannot start peak monitor stream: pipewire core is not ready.";
		return false;
	}

	auto target =
	    QByteArray::number(this->node->objectSerial ? this->node->objectSerial : this->node->id);

	// clang-format off
	auto* props = pw_properties_new(
	    PW_KEY_MEDIA_TYPE, "Audio",
	    PW_KEY_MEDIA_CATEGORY, "Monitor",
	    PW_KEY_MEDIA_NAME, "Peak detect",
	    PW_KEY_APP_NAME, "Quickshell Peak Detect",
	    PW_KEY_STREAM_MONITOR, "true",
		  PW_KEY_STREAM_CAPTURE_SINK, this->node->type.testFlags(PwNodeType::Sink) ? "true" : "false",
	    PW_KEY_TARGET_OBJECT, target.constData(),
	    nullptr
	);
	// clang-format on

	if (props == nullptr) {
		qCWarning(logPeak) << "Failed to create properties for peak monitor stream.";
		return false;
	}

	this->stream = pw_stream_new(core->core, "quickshell-peak-monitor", props);
	if (this->stream == nullptr) {
		qCWarning(logPeak) << "Failed to create peak monitor stream.";
		return false;
	}

	pw_stream_add_listener(this->stream, &this->listener.hook, &PwPeakStream::EVENTS, this);

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
		qCWarning(logPeak) << "Failed to connect peak monitor stream:" << res;
		this->destroy();
		return false;
	}

	return true;
}

void PwPeakStream::destroy() {
	if (this->stream == nullptr) return;
	this->listener.remove();
	pw_stream_destroy(this->stream);
	this->stream = nullptr;
	this->resetFormat();
}

void PwPeakStream::onProcess(void* data) {
	static_cast<PwPeakStream*>(data)->handleProcess(); // NOLINT
}

void PwPeakStream::onParamChanged(void* data, uint32_t id, const spa_pod* param) {
	static_cast<PwPeakStream*>(data)->handleParamChanged(id, param); // NOLINT
}

void PwPeakStream::onStateChanged(
    void* data,
    pw_stream_state oldState,
    pw_stream_state state,
    const char* error
) {
	static_cast<PwPeakStream*>(data)->handleStateChanged(oldState, state, error); // NOLINT
}

void PwPeakStream::onDestroy(void* data) {
	auto* self = static_cast<PwPeakStream*>(data); // NOLINT
	self->stream = nullptr;
	self->listener.remove();
	self->resetFormat();
}

void PwPeakStream::handleStateChanged(
    pw_stream_state oldState,
    pw_stream_state state,
    const char* error
) {
	if (state == PW_STREAM_STATE_ERROR) {
		if (error != nullptr) {
			qCWarning(logPeak) << "Peak monitor stream error:" << error;
		} else {
			qCWarning(logPeak) << "Peak monitor stream error.";
		}
	}

	if (state == PW_STREAM_STATE_PAUSED && oldState != PW_STREAM_STATE_PAUSED) {
		auto peakCount = this->monitor->mChannels.length();
		if (peakCount == 0) {
			peakCount = this->monitor->mPeaks.length();
		}
		if (peakCount == 0 && this->formatReady) {
			peakCount = static_cast<int>(this->format.channels);
		}

		if (peakCount > 0) {
			auto zeros = QVector<float>(peakCount, 0.0f);
			this->monitor->updatePeaks(zeros, 0.0f);
		}
	}
}

void PwPeakStream::handleParamChanged(uint32_t id, const spa_pod* param) {
	if (param == nullptr || id != SPA_PARAM_Format) return;

	auto info = spa_audio_info {};
	if (spa_format_parse(param, &info.media_type, &info.media_subtype) < 0) return;

	if (info.media_type != SPA_MEDIA_TYPE_audio || info.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		return;

	auto raw = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_UNKNOWN); // NOLINT
	if (spa_format_audio_raw_parse(param, &raw) < 0) return;

	if (raw.format != SPA_AUDIO_FORMAT_F32) {
		qCWarning(logPeak) << "Unsupported peak monitor format for" << this->node << ":" << raw.format;
		this->resetFormat();
		return;
	}

	this->format = raw;
	this->formatReady = raw.channels > 0;

	auto channels = QVector<PwAudioChannel::Enum>();
	channels.reserve(static_cast<int>(raw.channels));

	for (quint32 i = 0; i < raw.channels; i++) {
		if ((raw.flags & SPA_AUDIO_FLAG_UNPOSITIONED) != 0) {
			channels.push_back(PwAudioChannel::Unknown);
		} else {
			channels.push_back(static_cast<PwAudioChannel::Enum>(raw.position[i]));
		}
	}

	this->channelPeaks.fill(0.0f, channels.size());
	this->monitor->updateChannels(channels);
	this->monitor->updatePeaks(this->channelPeaks, 0.0f);
}

void PwPeakStream::resetFormat() {
	this->format = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_UNKNOWN);
	this->formatReady = false;
	this->channelPeaks.clear();
	this->monitor->clearPeaks();
}

void PwPeakStream::handleProcess() {
	if (!this->formatReady || this->stream == nullptr) return;

	auto* buffer = pw_stream_dequeue_buffer(this->stream);
	auto requeue = qScopeGuard([&, this] { pw_stream_queue_buffer(this->stream, buffer); });

	if (buffer == nullptr) {
		qCWarning(logPeak) << "Peak monitor ran out of buffers.";
		return;
	}

	auto* spaBuffer = buffer->buffer;
	if (spaBuffer == nullptr || spaBuffer->n_datas < 1) {
		return;
	}

	auto* data = &spaBuffer->datas[0]; // NOLINT
	if (data->data == nullptr || data->chunk == nullptr) {
		return;
	}

	auto channelCount = static_cast<int>(this->format.channels);
	if (channelCount <= 0) {
		return;
	}

	const auto* base = static_cast<const quint8*>(data->data) + data->chunk->offset; // NOLINT
	const auto* samples = reinterpret_cast<const float*>(base);
	auto sampleCount = static_cast<int>(data->chunk->size / sizeof(float));

	if (sampleCount < channelCount) {
		return;
	}

	QVector<float> volumes;
	if (auto* audioData = dynamic_cast<PwNodeBoundAudio*>(this->node->boundData)) {
		if (!this->node->shouldUseDevice()) volumes = audioData->volumes();
	}

	this->channelPeaks.fill(0.0f, channelCount);

	auto maxPeak = 0.0f;
	for (auto channel = 0; channel < channelCount; channel++) {
		auto peak = 0.0f;
		for (auto sample = channel; sample < sampleCount; sample += channelCount) {
			peak = std::max(peak, std::abs(samples[sample])); // NOLINT
		}

		auto visualPeak = std::cbrt(peak);
		if (!volumes.isEmpty() && volumes[channel] != 0.0f) visualPeak *= 1.0f / volumes[channel];

		this->channelPeaks[channel] = visualPeak;
		maxPeak = std::max(maxPeak, visualPeak);
	}

	this->monitor->updatePeaks(this->channelPeaks, maxPeak);
}

PwNodePeakMonitor::PwNodePeakMonitor(QObject* parent): QObject(parent) {}

PwNodePeakMonitor::~PwNodePeakMonitor() {
	delete this->mStream;
	this->mStream = nullptr;
}

PwNodeIface* PwNodePeakMonitor::node() const { return this->mNode; }

void PwNodePeakMonitor::setNode(PwNodeIface* node) {
	if (node == this->mNode) return;

	if (this->mNode != nullptr) {
		QObject::disconnect(this->mNode, nullptr, this, nullptr);
	}

	if (node != nullptr) {
		QObject::connect(node, &QObject::destroyed, this, &PwNodePeakMonitor::onNodeDestroyed);
	}

	this->mNode = node;
	this->mNodeRef.setObject(node != nullptr ? node->node() : nullptr);
	this->rebuildStream();
	emit this->nodeChanged();
}

bool PwNodePeakMonitor::isEnabled() const { return this->mEnabled; }

void PwNodePeakMonitor::setEnabled(bool enabled) {
	if (enabled == this->mEnabled) return;
	this->mEnabled = enabled;
	this->rebuildStream();
	emit this->enabledChanged();
}

void PwNodePeakMonitor::onNodeDestroyed() {
	this->mNode = nullptr;
	this->mNodeRef.setObject(nullptr);
	this->rebuildStream();
	emit this->nodeChanged();
}

void PwNodePeakMonitor::updatePeaks(const QVector<float>& peaks, float peak) {
	if (this->mPeaks != peaks) {
		this->mPeaks = peaks;
		emit this->peaksChanged();
	}

	if (this->mPeak != peak) {
		this->mPeak = peak;
		emit this->peakChanged();
	}
}

void PwNodePeakMonitor::updateChannels(const QVector<PwAudioChannel::Enum>& channels) {
	if (this->mChannels == channels) return;
	this->mChannels = channels;
	emit this->channelsChanged();
}

void PwNodePeakMonitor::clearPeaks() {
	if (!this->mPeaks.isEmpty()) {
		this->mPeaks.clear();
		emit this->peaksChanged();
	}

	if (!this->mChannels.isEmpty()) {
		this->mChannels.clear();
		emit this->channelsChanged();
	}

	if (this->mPeak != 0.0f) {
		this->mPeak = 0.0f;
		emit this->peakChanged();
	}
}

void PwNodePeakMonitor::rebuildStream() {
	delete this->mStream;
	this->mStream = nullptr;

	auto* node = this->mNodeRef.object();
	if (!this->mEnabled || node == nullptr) {
		this->clearPeaks();
		return;
	}

	if (node == nullptr || !node->type.testFlags(PwNodeType::Audio)) {
		this->clearPeaks();
		return;
	}

	this->mStream = new PwPeakStream(this, node);
	if (!this->mStream->start()) {
		delete this->mStream;
		this->mStream = nullptr;
		this->clearPeaks();
	}
}

} // namespace qs::service::pipewire

#pragma GCC diagnostic pop
