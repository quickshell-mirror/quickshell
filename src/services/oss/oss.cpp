#include "oss.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <fcntl.h>
#include <qalgorithms.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#ifdef __FreeBSD__
#include <sys/ioccom.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

namespace qs::service::oss {

// OSSMixerControl Implementation
OSSMixerControl::OSSMixerControl(
    QString name,
    int devId,
    int mixerId,
    MixerControlType type,
    QObject* parent
)
    : QObject(parent)
    , mName(std::move(name))
    , mDeviceId(devId)
    , mMixerId(mixerId)
    , mType(type) {
	this->refresh();
}

QString OSSMixerControl::name() const { return this->mName; }
int OSSMixerControl::left() const { return this->mLeft; }
int OSSMixerControl::right() const { return this->mRight; }
bool OSSMixerControl::isStereo() const { return this->mStereo; }
bool OSSMixerControl::isMuted() const { return this->mMuted; }
MixerControlType OSSMixerControl::type() const { return this->mType; }

void OSSMixerControl::setLeft(int value) {
	value = std::max(value, 0);
	value = std::min(value, 100);

	if (this->mLeft != value) {
		this->mLeft = value;
		this->writeValue();
		emit this->leftChanged();
	}
}

void OSSMixerControl::setRight(int value) {
	value = std::max(value, 0);
	value = std::min(value, 100);

	if (this->mRight != value) {
		this->mRight = value;
		this->writeValue();
		emit this->rightChanged();
	}
}

void OSSMixerControl::setMuted(bool muted) {
	if (this->mMuted != muted) {
		this->mMuted = muted;
		this->writeValue();
		emit this->mutedChanged();
	}
}

bool OSSMixerControl::refresh() { return this->updateValue(); }
bool OSSMixerControl::updateValue() {
	auto mixerPath = QString("/dev/mixer%1").arg(this->mDeviceId);
	auto fd = open(mixerPath.toUtf8().constData(), O_RDONLY);

	if (fd < 0) {
		qWarning() << "Failed to open mixer device:" << mixerPath;
		return false;
	}

	auto value = 0;

	if (ioctl(fd, MIXER_READ(this->mMixerId), &value) < 0) { // NOLINT
		close(fd);
		return false;
	}

	close(fd);

	auto newLeft = value & 0xFF;
	auto newRight = (value >> 8) & 0xFF;
	auto changed = false;

	if (this->mLeft != newLeft) {
		this->mLeft = newLeft;
		emit this->leftChanged();
		changed = true;
	}

	if (newRight != 0 && this->mRight != newRight) {
		this->mRight = newRight;
		this->mStereo = true;
		emit this->rightChanged();
		changed = true;
	} else if (newRight == 0 && !this->mStereo) {
		this->mRight = newLeft;
	}

	return changed;
}

bool OSSMixerControl::writeValue() {
	auto mixerPath = QString("/dev/mixer%1").arg(this->mDeviceId);
	auto fd = open(mixerPath.toUtf8().constData(), O_WRONLY);

	if (fd < 0) {
		qWarning() << "Failed to open mixer device for writing:" << mixerPath;
		return false;
	}

	auto left = this->mMuted ? 0 : this->mLeft;
	auto right = this->mMuted ? 0 : (this->mStereo ? this->mRight : this->mLeft);
	auto value = (right << 8) | left;

	if (ioctl(fd, MIXER_WRITE(this->mMixerId), &value) < 0) { // NOLINT
		close(fd);
		qWarning() << "Failed to write mixer value for control:" << this->mName;
		return false;
	}

	close(fd);
	return true;
}

// OSSSoundDevice Implementation

OSSSoundDevice::OSSSoundDevice(int deviceId, QObject* parent)
    : QObject(parent)
    , mDeviceId(deviceId)
    , mDescription(this->mName) {
	this->mName = QString("pcm%1").arg(deviceId);
}

int OSSSoundDevice::deviceId() const { return this->mDeviceId; }
QString OSSSoundDevice::name() const { return this->mName; }
QString OSSSoundDevice::description() const { return this->mDescription; }
DeviceMode OSSSoundDevice::mode() const { return this->mMode; }
bool OSSSoundDevice::isDefault() const { return this->mIsDefault; }
OSSMixerControl* OSSSoundDevice::master() const { return this->mMaster; }

bool OSSSoundDevice::initialize() {
	auto mixerPath = QString("/dev/mixer%1").arg(this->mDeviceId);
	auto fd = open(mixerPath.toUtf8().constData(), O_RDONLY);

	if (fd < 0) {
		qWarning() << "OSS: Failed to open mixer device:" << mixerPath
		           << "error:" << qt_error_string(errno);
		return false;
	}

	auto devmask = 0;

	if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) >= 0) { // NOLINT
		this->loadControls();
	} else {
		qWarning() << "OSS: Failed to read devmask for" << mixerPath;
		close(fd);
		return false;
	}

	auto sndstat = QFile("/dev/sndstat");

	if (sndstat.open(QIODevice::ReadOnly | QIODevice::Text)) {
		auto in = QTextStream(&sndstat);
		auto line = QString();

		while (in.readLineInto(&line)) {
			if (line.contains(QString("pcm%1:").arg(this->mDeviceId))) {
				auto start = line.indexOf('<');
				auto end = line.indexOf('>');

				if (start >= 0 && end > start) {
					this->mDescription = line.mid(start + 1, end - start - 1);
				}

				if (line.contains("(play/rec)")) {
					this->mMode = DeviceMode::PlaybackRecord;
				} else if (line.contains("(play)")) {
					this->mMode = DeviceMode::Playback;
				} else if (line.contains("(rec)")) {
					this->mMode = DeviceMode::Record;
				}

				if (line.contains("default")) {
					this->mIsDefault = true;
				}

				break;
			}
		}

		sndstat.close();
	} else {
		qWarning() << "OSS: Could not open /dev/sndstat";
	}

	close(fd);
	return true;
}

void OSSSoundDevice::loadControls() {
	auto mixerPath = QString("/dev/mixer%1").arg(this->mDeviceId);
	auto fd = open(mixerPath.toUtf8().constData(), O_RDONLY);

	if (fd < 0) return;

	auto devmask = 0;

	if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0) {
		close(fd);
		return;
	}

	auto recmask = 0;
	ioctl(fd, SOUND_MIXER_READ_RECMASK, &recmask); // NOLINT

	const char* labels[] = SOUND_DEVICE_LABELS; // NOLINT
	const auto numDevs = SOUND_MIXER_NRDEVICES; // NOLINT

	for (auto i = 0; i < numDevs; i++) {
		if (devmask & (1 << i)) {
			auto name = QString::fromLatin1(labels[i]);
			auto type = MixerControlType::Volume;

			auto* control = new OSSMixerControl(name, this->mDeviceId, i, type, this);
			this->mControls.append(control);

			if (!this->mMaster && (name == "vol" || name == "pcm")) {
				this->mMaster = control;
			}
		}
	}

	close(fd);

	if (!this->mMaster && !this->mControls.isEmpty()) {
		this->mMaster = this->mControls.first();
	}

	emit this->controlsChanged();
	emit this->masterChanged();
}

void OSSSoundDevice::setIsDefault(bool isDefault) {
	if (this->mIsDefault != isDefault) {
		this->mIsDefault = isDefault;
		emit this->isDefaultChanged();
	}
}

void OSSSoundDevice::refresh() {
	for (auto* control: this->mControls) {
		control->refresh();
	}
}

QList<QObject*> OSSSoundDevice::controls() const {
	auto list = QList<QObject*>();
	list.reserve(this->mControls.size());

	for (auto* control: this->mControls) {
		list.append(control);
	}

	return list;
}

// OSS Implementation

OSS::OSS(QObject* parent): QObject(parent), mPollTimer(new QTimer(this)) {
	if (QFile::exists("/dev/sndstat")) {
		this->mAvailable = true;

		this->scanDevices();
		this->updateDefault();

		connect(this->mPollTimer, &QTimer::timeout, this, &OSS::refresh);
		this->mPollTimer->start(2000);
	} else {
		qWarning() << "OSS sound system is not available";
	}
}

QList<QObject*> OSS::devices() const {
	auto list = QList<QObject*>();
	list.reserve(this->mDevices.size());

	for (auto* device: this->mDevices) {
		list.append(device);
	}

	return list;
}

OSSSoundDevice* OSS::defaultDevice() const { return this->mDefaultDevice; }
bool OSS::isAvailable() const { return this->mAvailable; }

void OSS::scanDevices() {
	qDeleteAll(this->mDevices);
	this->mDevices.clear();
	this->mDefaultDevice = nullptr;

	for (auto i = 0; i < 32; i++) {
		auto mixerPath = QString("/dev/mixer%1").arg(i);

		if (QFile::exists(mixerPath)) {
			auto* device = new OSSSoundDevice(i, this);

			if (device->initialize()) {
				this->mDevices.append(device);
			} else {
				qWarning() << "OSS: Failed to initialize device:" << mixerPath;
				delete device;
			}
		}
	}

	emit this->devicesChanged();
}

void OSS::updateDefault() {
	auto* newDefault = static_cast<OSSSoundDevice*>(nullptr);
	auto sndstat = QFile("/dev/sndstat");

	if (sndstat.open(QIODevice::ReadOnly | QIODevice::Text)) {
		auto in = QTextStream(&sndstat);
		auto line = QString();

		while (in.readLineInto(&line)) {
			if (line.contains("default")) {
				auto pcmIdx = line.indexOf("pcm");

				if (pcmIdx >= 0) {
					auto numStr = QString();

					for (auto i = pcmIdx + 3; i < line.length(); i++) {
						if (line[i].isDigit()) {
							numStr += line[i];
						} else {
							break;
						}
					}

					if (!numStr.isEmpty()) {
						auto devId = numStr.toInt();

						for (auto* device: this->mDevices) {
							if (device->deviceId() == devId) {
								newDefault = device;
								device->setIsDefault(true);
							} else {
								device->setIsDefault(false);
							}
						}
					}
				}

				break;
			}
		}

		sndstat.close();
	} else {
		qWarning() << "OSS: Failed to open /dev/sndstat for reading default device";
	}

	if (!newDefault && !this->mDevices.isEmpty()) {
		newDefault = this->mDevices.first();
		newDefault->setIsDefault(true);
	}

	if (this->mDefaultDevice != newDefault) {
		this->mDefaultDevice = newDefault;
		emit this->defaultDeviceChanged();
	}
}

bool OSS::setDefaultDevice(int deviceId) {
	int value = deviceId;
	size_t size = sizeof(value);

	if (sysctlbyname("hw.snd.default_unit", nullptr, nullptr, &value, size) == 0) {
		this->updateDefault();
		return true;
	}

	qWarning() << "Failed to set default device to" << deviceId << "error:" << qt_error_string(errno);

	return false;
}

void OSS::refresh() {
	this->updateDefault();

	for (auto* device: this->mDevices) {
		device->refresh();
	}
}

} // namespace qs::service::oss
