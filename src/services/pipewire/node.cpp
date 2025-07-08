#include "node.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <pipewire/core.h>
#include <pipewire/keys.h>
#include <pipewire/node.h>
#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringliteral.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/node/keys.h>
#include <spa/param/param.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>
#include <spa/pod/pod.h>
#include <spa/pod/vararg.h>
#include <spa/utils/dict.h>
#include <spa/utils/keys.h>
#include <spa/utils/type.h>

#include "../../core/logcat.hpp"
#include "connection.hpp"
#include "core.hpp"
#include "device.hpp"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logNode, "quickshell.service.pipewire.node", QtWarningMsg);
}

QString PwAudioChannel::toString(Enum value) {
	switch (value) {
	case Unknown: return "Unknown";
	case NA: return "N/A";
	case Mono: return "Mono";
	case FrontCenter: return "Front Center";
	case FrontLeft: return "Front Left";
	case FrontRight: return "Front Right";
	case FrontLeftCenter: return "Front Left Center";
	case FrontRightCenter: return "Front Right Center";
	case FrontLeftWide: return "Front Left Wide";
	case FrontRightWide: return "Front Right Wide";
	case FrontCenterHigh: return "Front Center High";
	case FrontLeftHigh: return "Front Left High";
	case FrontRightHigh: return "Front Right High";
	case LowFrequencyEffects: return "Low Frequency Effects";
	case LowFrequencyEffects2: return "Low Frequency Effects 2";
	case LowFrequencyEffectsLeft: return "Low Frequency Effects Left";
	case LowFrequencyEffectsRight: return "Low Frequency Effects Right";
	case SideLeft: return "Side Left";
	case SideRight: return "Side Right";
	case RearCenter: return "Rear Center";
	case RearLeft: return "Rear Left";
	case RearRight: return "Rear Right";
	case RearLeftCenter: return "Rear Left Center";
	case RearRightCenter: return "Rear Right Center";
	case TopCenter: return "Top Center";
	case TopFrontCenter: return "Top Front Center";
	case TopFrontLeft: return "Top Front Left";
	case TopFrontRight: return "Top Front Right";
	case TopFrontLeftCenter: return "Top Front Left Center";
	case TopFrontRightCenter: return "Top Front Right Center";
	case TopSideLeft: return "Top Side Left";
	case TopSideRight: return "Top Side Right";
	case TopRearCenter: return "Top Rear Center";
	case TopRearLeft: return "Top Rear Left";
	case TopRearRight: return "Top Rear Right";
	case BottomCenter: return "Bottom Center";
	case BottomLeftCenter: return "Bottom Left Center";
	case BottomRightCenter: return "Bottom Right Center";
	default:
		if (value >= AuxRangeStart && value <= AuxRangeEnd) {
			return QString("Aux %1").arg(value - AuxRangeStart + 1);
		} else if (value >= CustomRangeStart) {
			return QString("Custom %1").arg(value - CustomRangeStart + 1);
		} else {
			return "Unknown";
		}
	}
}

QString PwNodeType::toString(PwNodeType::Flags type) {
	switch (type) {
	case PwNodeType::VideoSource: return QStringLiteral("VideoSource");
	case PwNodeType::VideoSink: return QStringLiteral("VideoSink");
	case PwNodeType::AudioSource: return QStringLiteral("AudioSource");
	case PwNodeType::AudioSink: return QStringLiteral("AudioSink");
	case PwNodeType::AudioDuplex: return QStringLiteral("AudioDuplex");
	case PwNodeType::AudioOutStream: return QStringLiteral("AudioOutStream");
	case PwNodeType::AudioInStream: return QStringLiteral("AudioInStream");
	case PwNodeType::Untracked: return QStringLiteral("Untracked");
	default: return QStringLiteral("Invalid");
	}
}

void PwNode::bindHooks() {
	// Bind the device first as pw is in order, meaning the device should be bound before
	// we want to do anything with it.
	if (this->device) this->device->ref();

	pw_node_add_listener(this->proxy(), &this->listener.hook, &PwNode::EVENTS, this);
}

void PwNode::unbindHooks() {
	if (this->ready) {
		this->ready = false;
		emit this->readyChanged();
	}

	this->syncSeq = 0;
	this->listener.remove();
	this->routeDevice = -1;
	this->properties.clear();
	emit this->propertiesChanged();

	if (this->boundData != nullptr) {
		this->boundData->onUnbind();
	}

	// unbind after the node is unbound
	if (this->device) this->device->unref();
}

void PwNode::initProps(const spa_dict* props) {
	if (const auto* mediaClass = spa_dict_lookup(props, SPA_KEY_MEDIA_CLASS)) {
		if (strcmp(mediaClass, "Audio/Sink") == 0) {
			this->type = PwNodeType::AudioSink;
		} else if (strcmp(mediaClass, "Audio/Source") == 0) {
			this->type = PwNodeType::AudioSource;
		} else if (strcmp(mediaClass, "Audio/Duplex") == 0) {
			this->type = PwNodeType::AudioDuplex;
		} else if (strcmp(mediaClass, "Stream/Output/Audio") == 0) {
			this->type = PwNodeType::AudioOutStream;
		} else if (strcmp(mediaClass, "Stream/Input/Audio") == 0) {
			this->type = PwNodeType::AudioInStream;
		} else if (strcmp(mediaClass, "Video/Sink") == 0) {
			this->type = PwNodeType::VideoSink;
		} else if (strcmp(mediaClass, "Video/Source") == 0) {
			this->type = PwNodeType::VideoSource;
		}
	}

	if (const auto* nodeName = spa_dict_lookup(props, SPA_KEY_NODE_NAME)) {
		this->name = nodeName;
	}

	if (const auto* nodeDesc = spa_dict_lookup(props, SPA_KEY_NODE_DESCRIPTION)) {
		this->description = nodeDesc;
	}

	if (const auto* nodeNick = spa_dict_lookup(props, PW_KEY_NODE_NICK)) {
		this->nick = nodeNick;
	}

	if (const auto* deviceId = spa_dict_lookup(props, PW_KEY_DEVICE_ID)) {
		auto ok = false;
		auto id = QString::fromUtf8(deviceId).toInt(&ok);

		if (!ok) {
			qCCritical(logNode) << this << "has a device.id property but the value is not an integer. Id:"
			                    << deviceId;
		} else {
			this->device = this->registry->devices.value(id);

			if (this->device == nullptr) {
				qCCritical(logNode
				) << this
				  << "has a device.id property that does not corrospond to a device object. Id:" << id;
			}
		}
	}

	if (this->type.testFlags(PwNodeType::Audio)) {
		this->boundData = new PwNodeBoundAudio(this);
	}
}

const pw_node_events PwNode::EVENTS = {
    .version = PW_VERSION_NODE_EVENTS,
    .info = &PwNode::onInfo,
    .param = &PwNode::onParam,
};

void PwNode::onInfo(void* data, const pw_node_info* info) {
	auto* self = static_cast<PwNode*>(data);

	if ((info->change_mask & PW_NODE_CHANGE_MASK_PROPS) != 0) {
		auto properties = QMap<QString, QString>();

		if (self->device) {
			if (const auto* routeDevice = spa_dict_lookup(info->props, "card.profile.device")) {
				auto ok = false;
				auto id = QString::fromUtf8(routeDevice).toInt(&ok);

				if (!ok) {
					qCCritical(logNode
					) << self
					  << "has a card.profile.device property but the value is not an integer. Value:" << id;
				}

				self->routeDevice = id;
			} else {
				qCCritical(logNode) << self << "has attached device" << self->device
				                    << "but no card.profile.device property.";
			}
		}

		const spa_dict_item* item = nullptr;
		spa_dict_for_each(item, info->props) { properties.insert(item->key, item->value); }

		self->properties = properties;
		emit self->propertiesChanged();
	}

	if (self->boundData != nullptr) {
		self->boundData->onInfo(info);
	}

	if (!self->ready && !self->syncSeq) {
		auto* core = PwConnection::instance()->registry.core;
		QObject::connect(core, &PwCore::synced, self, &PwNode::onCoreSync);
		self->syncSeq = core->sync(self->id);
	}
}

void PwNode::onCoreSync(quint32 id, qint32 seq) {
	if (id != this->id || seq != this->syncSeq) return;
	qCInfo(logNode) << "Completed initial sync for" << this;
	this->ready = true;
	this->syncSeq = 0;
	emit this->readyChanged();
}

void PwNode::onParam(
    void* data,
    qint32 /*seq*/,
    quint32 id,
    quint32 index,
    quint32 /*next*/,
    const spa_pod* param
) {
	auto* self = static_cast<PwNode*>(data);
	if (self->boundData != nullptr) {
		self->boundData->onSpaParam(id, index, param);
	}
}

PwNodeBoundAudio::PwNodeBoundAudio(PwNode* node): QObject(node), node(node) {
	if (node->device) {
		QObject::connect(node->device, &PwDevice::deviceReady, this, &PwNodeBoundAudio::onDeviceReady);

		QObject::connect(
		    node->device,
		    &PwDevice::routeVolumesChanged,
		    this,
		    &PwNodeBoundAudio::onDeviceVolumesChanged
		);
	}
}

void PwNodeBoundAudio::onInfo(const pw_node_info* info) {
	if ((info->change_mask & PW_NODE_CHANGE_MASK_PARAMS) != 0) {
		for (quint32 i = 0; i < info->n_params; i++) {
			auto& param = info->params[i]; // NOLINT

			if (param.id == SPA_PARAM_Props) {
				if ((param.flags & SPA_PARAM_INFO_READWRITE) == SPA_PARAM_INFO_READWRITE) {
					qCDebug(logNode) << "Enumerating props param for" << this->node;
					pw_node_enum_params(this->node->proxy(), 0, param.id, 0, UINT32_MAX, nullptr);
				} else {
					qCWarning(logNode) << "Unable to enumerate props param for" << this->node
					                   << "as the param does not have read+write permissions.";
				}
			}
		}
	}
}

void PwNodeBoundAudio::onSpaParam(quint32 id, quint32 index, const spa_pod* param) {
	if (id == SPA_PARAM_Props && index == 0) {
		if (this->node->device) {
			qCDebug(logNode) << "Skipping node volume props update for" << this->node
			                 << "in favor of device updates.";
			return;
		}

		this->updateVolumeProps(PwVolumeProps::parseSpaPod(param));
	}
}

void PwNodeBoundAudio::updateVolumeProps(const PwVolumeProps& volumeProps) {
	if (volumeProps.volumes.size() != volumeProps.channels.size()) {
		qCWarning(logNode) << "Cannot update volume props of" << this->node
		                   << "- channelVolumes and channelMap are not the same size. Sizes:"
		                   << volumeProps.volumes.size() << volumeProps.channels.size();
		return;
	}

	// It is important that the lengths of channels and volumes stay in sync whenever you read them.
	auto channelsChanged = false;
	auto volumesChanged = false;
	auto mutedChanged = false;

	if (this->mChannels != volumeProps.channels) {
		this->mChannels = volumeProps.channels;
		channelsChanged = true;
		qCInfo(logNode) << "Got updated channels of" << this->node << '-' << this->mChannels;
	}

	if (this->mServerVolumes != volumeProps.volumes) {
		this->mServerVolumes = volumeProps.volumes;
	}

	if (this->mVolumes != volumeProps.volumes) {
		this->mVolumes = volumeProps.volumes;
		volumesChanged = true;
		qCInfo(logNode) << "Got updated volumes of" << this->node << '-' << this->mVolumes;
	}

	if (volumeProps.mute != this->mMuted) {
		this->mMuted = volumeProps.mute;
		mutedChanged = true;
		qCInfo(logNode) << "Got updated mute status of" << this->node << '-' << volumeProps.mute;
	}

	if (channelsChanged) emit this->channelsChanged();
	if (volumesChanged) emit this->volumesChanged();
	if (mutedChanged) emit this->mutedChanged();
}

void PwNodeBoundAudio::onUnbind() {
	this->mChannels.clear();
	this->mVolumes.clear();
	this->mServerVolumes.clear();
	this->mDeviceVolumes.clear();
	this->waitingVolumes.clear();
	emit this->channelsChanged();
	emit this->volumesChanged();
}

bool PwNodeBoundAudio::isMuted() const { return this->mMuted; }

void PwNodeBoundAudio::setMuted(bool muted) {
	if (this->node->proxy() == nullptr) {
		qCCritical(logNode) << "Tried to change mute state for" << this->node << "which is not bound.";
		return;
	}

	if (muted == this->mMuted) return;

	if (this->node->device) {
		qCInfo(logNode) << "Changing muted state of" << this->node << "to" << muted << "via device";
		if (!this->node->device->setMuted(this->node->routeDevice, muted)) {
			return;
		}
	} else {
		auto buffer = std::array<quint8, 1024>();
		auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());

		// is this a leak? seems like probably not? docs don't say, as usual.
		// clang-format off
		auto* pod = spa_pod_builder_add_object(
				&builder, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
				SPA_PROP_mute, SPA_POD_Bool(muted)
		);
		// clang-format on

		qCInfo(logNode) << "Changed muted state of" << this->node << "to" << muted << "via node";
		pw_node_set_param(this->node->proxy(), SPA_PARAM_Props, 0, static_cast<spa_pod*>(pod));
	}

	this->mMuted = muted;
	emit this->mutedChanged();
}

float PwNodeBoundAudio::averageVolume() const {
	float total = 0;

	for (auto volume: this->mVolumes) {
		total += volume;
	}

	return total / static_cast<float>(this->mVolumes.size());
}

void PwNodeBoundAudio::setAverageVolume(float volume) {
	auto oldAverage = this->averageVolume();
	auto mul = oldAverage == 0 ? 0 : volume / oldAverage;
	auto volumes = QVector<float>();

	for (auto oldVolume: this->mVolumes) {
		volumes.push_back(mul == 0 ? volume : oldVolume * mul);
	}

	this->setVolumes(volumes);
}

QVector<PwAudioChannel::Enum> PwNodeBoundAudio::channels() const { return this->mChannels; }

QVector<float> PwNodeBoundAudio::volumes() const { return this->mVolumes; }

void PwNodeBoundAudio::setVolumes(const QVector<float>& volumes) {
	if (this->node->proxy() == nullptr) {
		qCCritical(logNode) << "Tried to change node volumes for" << this->node
		                    << "which is not bound.";
		return;
	}

	auto realVolumes = QVector<float>();
	for (auto volume: volumes) {
		realVolumes.push_back(volume < 0 ? 0 : volume);
	}

	if (realVolumes == this->mVolumes) return;

	if (realVolumes.length() != this->mVolumes.length()) {
		qCCritical(logNode) << "Tried to change node volumes for" << this->node << "from"
		                    << this->mVolumes << "to" << volumes
		                    << "which has a different length than the list of channels"
		                    << this->mChannels;
		return;
	}

	if (this->node->device) {
		if (this->node->device->waitingForDevice()) {
			qCInfo(logNode) << "Waiting to change volumes of" << this->node << "to" << realVolumes
			                << "via device";
			this->waitingVolumes = realVolumes;
		} else {
			auto significantChange = this->mServerVolumes.isEmpty();
			for (auto i = 0; i < this->mServerVolumes.length(); i++) {
				auto serverVolume = this->mServerVolumes.value(i);
				auto targetVolume = realVolumes.value(i);
				if (targetVolume == 0 || abs(targetVolume - serverVolume) >= 0.0001) {
					significantChange = true;
					break;
				}
			}

			if (significantChange) {
				qCInfo(logNode) << "Changing volumes of" << this->node << "to" << realVolumes
				                << "via device";
				if (!this->node->device->setVolumes(this->node->routeDevice, realVolumes)) {
					return;
				}

				this->mDeviceVolumes = realVolumes;
				this->node->device->waitForDevice();
			} else {
				// Insignificant changes won't cause an info event on the device, leaving qs hung in the
				// "waiting for acknowledgement" state forever.
				qCInfo(logNode) << "Ignoring volume change for" << this->node << "to" << realVolumes
				                << "from" << this->mServerVolumes
				                << "as it is a device node and the change is too small.";
			}
		}
	} else {
		auto buffer = std::array<quint8, 1024>();
		auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());

		auto cubedVolumes = QVector<float>();
		for (auto volume: realVolumes) {
			cubedVolumes.push_back(volume * volume * volume);
		}

		// clang-format off
		auto* pod = spa_pod_builder_add_object(
				&builder, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
				SPA_PROP_channelVolumes, SPA_POD_Array(sizeof(float), SPA_TYPE_Float, cubedVolumes.length(), cubedVolumes.data())
		);
		// clang-format on

		qCInfo(logNode) << "Changing volumes of" << this->node << "to" << volumes << "via node";
		pw_node_set_param(this->node->proxy(), SPA_PARAM_Props, 0, static_cast<spa_pod*>(pod));
	}

	this->mVolumes = realVolumes;
	emit this->volumesChanged();
}

void PwNodeBoundAudio::onDeviceReady() {
	if (!this->waitingVolumes.isEmpty()) {
		if (this->waitingVolumes != this->mDeviceVolumes) {
			qCInfo(logNode) << "Changing volumes of" << this->node << "to" << this->waitingVolumes
			                << "via device (delayed)";

			this->node->device->setVolumes(this->node->routeDevice, this->waitingVolumes);
			this->mDeviceVolumes = this->waitingVolumes;
			this->mVolumes = this->waitingVolumes;
		}

		this->waitingVolumes.clear();
	}
}

void PwNodeBoundAudio::onDeviceVolumesChanged(
    qint32 routeDevice,
    const PwVolumeProps& volumeProps
) {
	if (this->node->device && this->node->routeDevice == routeDevice) {
		qCDebug(logNode) << "Got updated device volume props for" << this->node << "via"
		                 << this->node->device;

		this->updateVolumeProps(volumeProps);
	}
}

PwVolumeProps PwVolumeProps::parseSpaPod(const spa_pod* param) {
	auto props = PwVolumeProps();

	const auto* volumesProp = spa_pod_find_prop(param, nullptr, SPA_PROP_channelVolumes);
	const auto* channelsProp = spa_pod_find_prop(param, nullptr, SPA_PROP_channelMap);
	const auto* muteProp = spa_pod_find_prop(param, nullptr, SPA_PROP_mute);

	const auto* volumes = reinterpret_cast<const spa_pod_array*>(&volumesProp->value);
	const auto* channels = reinterpret_cast<const spa_pod_array*>(&channelsProp->value);

	spa_pod* iter = nullptr;
	SPA_POD_ARRAY_FOREACH(volumes, iter) {
		// Cubing behavior found in MPD source, and appears to corrospond to everyone else's measurements correctly.
		auto linear = *reinterpret_cast<float*>(iter);
		auto visual = std::cbrt(linear);
		props.volumes.push_back(visual);
	}

	SPA_POD_ARRAY_FOREACH(channels, iter) {
		props.channels.push_back(*reinterpret_cast<PwAudioChannel::Enum*>(iter));
	}

	spa_pod_get_bool(&muteProp->value, &props.mute);

	return props;
}

} // namespace qs::service::pipewire
