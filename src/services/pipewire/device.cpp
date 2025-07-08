#include "device.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <utility>

#include <pipewire/device.h>
#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/param/param.h>
#include <spa/param/props.h>
#include <spa/param/route.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <spa/pod/pod.h>
#include <spa/pod/vararg.h>
#include <spa/utils/type.h>

#include "../../core/logcat.hpp"
#include "core.hpp"
#include "node.hpp"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logDevice, "quickshell.service.pipewire.device", QtWarningMsg);
}

// https://github.com/PipeWire/wireplumber/blob/895c1c7286e8809fad869059179e53ab39c807e9/modules/module-mixer-api.c#L397
// https://github.com/PipeWire/pipewire/blob/48c2e9516585ccc791335bc7baf4af6952ec54a0/src/modules/module-protocol-pulse/pulse-server.c#L2743-L2743

void PwDevice::bindHooks() {
	pw_device_add_listener(this->proxy(), &this->listener.hook, &PwDevice::EVENTS, this);
	QObject::connect(this->registry->core, &PwCore::polled, this, &PwDevice::polled);
}

void PwDevice::unbindHooks() {
	QObject::disconnect(this->registry->core, &PwCore::polled, this, &PwDevice::polled);
	this->listener.remove();
	this->stagingIndexes.clear();
	this->routeDeviceIndexes.clear();
	this->mWaitingForDevice = false;
}

const pw_device_events PwDevice::EVENTS = {
    .version = PW_VERSION_DEVICE_EVENTS,
    .info = &PwDevice::onInfo,
    .param = &PwDevice::onParam,
};

void PwDevice::onInfo(void* data, const pw_device_info* info) {
	auto* self = static_cast<PwDevice*>(data);

	if ((info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS) == PW_DEVICE_CHANGE_MASK_PARAMS) {
		for (quint32 i = 0; i != info->n_params; i++) {
			auto& param = info->params[i]; // NOLINT

			if (param.id == SPA_PARAM_Route) {
				if ((param.flags & SPA_PARAM_INFO_READWRITE) == SPA_PARAM_INFO_READWRITE) {
					qCDebug(logDevice) << "Enumerating routes param for" << self;
					self->stagingIndexes.clear();
					self->deviceResponded = false;
					pw_device_enum_params(self->proxy(), 0, param.id, 0, UINT32_MAX, nullptr);
				} else {
					qCWarning(logDevice) << "Unable to enumerate route param for" << self
					                     << "as the param does not have read+write permissions.";
				}

				break;
			}
		}
	}
}

void PwDevice::onParam(
    void* data,
    qint32 /*seq*/,
    quint32 id,
    quint32 /*index*/,
    quint32 /*next*/,
    const spa_pod* param
) {
	auto* self = static_cast<PwDevice*>(data);

	if (id == SPA_PARAM_Route) {
		if (!self->deviceResponded) {
			self->deviceResponded = true;

			if (self->mWaitingForDevice) {
				self->mWaitingForDevice = false;
				emit self->deviceReady();
			}
		}

		self->addDeviceIndexPairs(param);
	}
}

void PwDevice::addDeviceIndexPairs(const spa_pod* param) {
	auto parser = spa_pod_parser();
	spa_pod_parser_pod(&parser, param);

	qint32 device = 0;
	qint32 index = 0;

	spa_pod* props = nullptr;

	// clang-format off
	quint32 id = SPA_PARAM_Route;
	spa_pod_parser_get_object(
			&parser, SPA_TYPE_OBJECT_ParamRoute, &id,
			SPA_PARAM_ROUTE_device, SPA_POD_Int(&device),
			SPA_PARAM_ROUTE_index, SPA_POD_Int(&index),
			SPA_PARAM_ROUTE_props, SPA_POD_PodObject(&props)
	);
	// clang-format on

	auto volumeProps = PwVolumeProps::parseSpaPod(props);

	this->stagingIndexes.append(device);
	// Insert into the main map as well, staging's purpose is to remove old entries.
	this->routeDeviceIndexes.insert(device, index);

	qCDebug(logDevice).nospace() << "Registered device/index pair for " << this
	                             << ": [device: " << device << ", index: " << index << ']';

	emit this->routeVolumesChanged(device, volumeProps);
}

void PwDevice::polled() {
	// It is far more likely that the list content has not come in yet than it having no entries,
	// and there isn't a way to check in the case that there *aren't* actually any entries.
	if (!this->stagingIndexes.isEmpty()) {
		this->routeDeviceIndexes.removeIf([&](const std::pair<qint32, qint32>& entry) {
			if (!stagingIndexes.contains(entry.first)) {
				qCDebug(logDevice).nospace() << "Removed device/index pair [device: " << entry.first
				                             << ", index: " << entry.second << "] for" << this;
				return true;
			}

			return false;
		});
	}
}

bool PwDevice::setVolumes(qint32 routeDevice, const QVector<float>& volumes) {
	return this->setRouteProps(routeDevice, [this, routeDevice, &volumes](spa_pod_builder* builder) {
		auto cubedVolumes = QVector<float>();
		for (auto volume: volumes) {
			cubedVolumes.push_back(volume * volume * volume);
		}

		// clang-format off
		auto* props = spa_pod_builder_add_object(
				builder, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
				SPA_PROP_channelVolumes, SPA_POD_Array(sizeof(float), SPA_TYPE_Float, cubedVolumes.length(), cubedVolumes.data())
		);
		// clang-format on

		qCInfo(logDevice) << "Changing volumes of" << this << "on route device" << routeDevice << "to"
		                  << volumes;
		return props;
	});
}

bool PwDevice::setMuted(qint32 routeDevice, bool muted) {
	return this->setRouteProps(routeDevice, [this, routeDevice, muted](spa_pod_builder* builder) {
		// clang-format off
		auto* props = spa_pod_builder_add_object(
				builder, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
				SPA_PROP_mute, SPA_POD_Bool(muted)
		);
		// clang-format on

		qCInfo(logDevice) << "Changing muted state of" << this << "on route device" << routeDevice
		                  << "to" << muted;
		return props;
	});
}

void PwDevice::waitForDevice() { this->mWaitingForDevice = true; }
bool PwDevice::waitingForDevice() const { return this->mWaitingForDevice; }

bool PwDevice::setRouteProps(
    qint32 routeDevice,
    const std::function<void*(spa_pod_builder*)>& propsCallback
) {
	if (this->proxy() == nullptr) {
		qCCritical(logDevice) << "Tried to change device route props for" << this
		                      << "which is not bound.";
		return false;
	}

	if (!this->routeDeviceIndexes.contains(routeDevice)) {
		qCCritical(logDevice) << "Tried to change device route props for" << this
		                      << "with untracked route device" << routeDevice;
		return false;
	}

	auto routeIndex = this->routeDeviceIndexes.value(routeDevice);

	auto buffer = std::array<quint8, 1024>();
	auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());

	auto* props = propsCallback(&builder);

	// clang-format off
	auto* route = spa_pod_builder_add_object(
	    &builder, SPA_TYPE_OBJECT_ParamRoute, SPA_PARAM_Route,
			SPA_PARAM_ROUTE_device, SPA_POD_Int(routeDevice),
			SPA_PARAM_ROUTE_index, SPA_POD_Int(routeIndex),
			SPA_PARAM_ROUTE_props, SPA_POD_PodObject(props),
			SPA_PARAM_ROUTE_save, SPA_POD_Bool(true)
	);
	// clang-format on

	pw_device_set_param(this->proxy(), SPA_PARAM_Route, 0, static_cast<spa_pod*>(route));
	return true;
}

} // namespace qs::service::pipewire
