#include "oss.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <QFile>
#include <QFileDevice>
#include <QHash>
#include <QList>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSocketNotifier>
#include <QTextStream>
#include <QTimer>
#include <Qt>
#include <QtAlgorithms>
#include <QtGlobal>
#include <QtLogging>
#include <QtTypes>
#include <fcntl.h>
#ifdef __FreeBSD__
#include <sys/event.h>
#include <sys/ioccom.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/un.h>
#endif
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#include "../../core/logcat.hpp"

namespace {
QS_LOGGING_CATEGORY(logOSS, "quickshell.service.oss", QtWarningMsg);
}

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
		qCWarning(logOSS) << "Failed to open mixer device:" << mixerPath;
		return false;
	}

	auto value = 0;
	auto changed = false;

	if (ioctl(fd, MIXER_READ(this->mMixerId), &value) < 0) { // NOLINT
		close(fd);
		qCWarning(logOSS) << "Failed to read mixer";
		return false;
	}

	auto newLeft = value & 0xFF;
	auto newRight = (value >> 8) & 0xFF;
	auto muteValue = 0;
	bool newMuted = false;

	if (ioctl(fd, SOUND_MIXER_READ_MUTE, &muteValue) >= 0) { // NOLINT
		newMuted = (muteValue & (1 << this->mMixerId)) != 0;
	}

	close(fd);

	if (this->mMuted != newMuted) {
		this->mMuted = newMuted;
		emit this->mutedChanged();
		changed = true;
	}

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
		qCWarning(logOSS) << "Failed to open mixer device for writing:" << mixerPath;
		return false;
	}

	auto left = this->mLeft;
	auto right = this->mStereo ? this->mRight : this->mLeft;
	auto value = (right << 8) | left;

	if (ioctl(fd, MIXER_WRITE(this->mMixerId), &value) < 0) { // NOLINT
		close(fd);
		qCWarning(logOSS) << "Failed to write mixer value for control:" << this->mName;
		return false;
	}

	auto muteValue = 0;

	if (ioctl(fd, SOUND_MIXER_READ_MUTE, &muteValue) >= 0) { // NOLINT
		if (this->mMuted) {
			muteValue |= (1 << this->mMixerId);
		} else {
			muteValue &= ~(1 << this->mMixerId);
		}

		if (ioctl(fd, SOUND_MIXER_WRITE_MUTE, &muteValue) < 0) { // NOLINT
			qCWarning(logOSS) << "Failed to write mute state for control:" << this->mName;
		}
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
		qCWarning(logOSS) << "Failed to open mixer device:" << mixerPath
		                  << "error:" << qt_error_string(errno);
		return false;
	}

	auto devmask = 0;

	if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) >= 0) { // NOLINT
		this->loadControls();
	} else {
		qCWarning(logOSS) << "Failed to read devmask for" << mixerPath;
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
		qCWarning(logOSS) << "Could not open /dev/sndstat";
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
OSS::OSS(QObject* parent): QObject(parent), mRescanTimer(new QTimer(this)) {
	if (QFile::exists("/dev/sndstat")) {
		this->mAvailable = true;

		this->scanDevices();
		this->updateDefault();

		this->mRescanTimer->setSingleShot(true);
		this->mRescanTimer->setInterval(100); // debounce

		connect(this->mRescanTimer, &QTimer::timeout, this, [this]() {
			this->scanDevices();
			this->updateDefault();
		});

		this->connectToDevd();
		this->initializeJackStates();
		// clang-format off
		#ifdef __FreeBSD__
		this->setupJackDetection();
		#endif
		// clang-format on
	} else {
		qCWarning(logOSS) << "OSS sound system is not available";
	}
}

OSS::~OSS() {
	if (this->mKqueueNotifier) {
		this->mKqueueNotifier->setEnabled(false);
	}

	if (this->mKqueue >= 0) {
		close(this->mKqueue);
	}

	if (this->mLogFileDescriptor >= 0) {
		close(this->mLogFileDescriptor);
	}

	if (this->mDevdSocket >= 0) {
		close(this->mDevdSocket);
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

void OSS::connectToDevd() {
	// clang-format off
	#ifdef __FreeBSD__
	// clang-format on
	// Try seqpacket first
	const char* pipePaths[] = {
	    "/var/run/devd.seqpacket.pipe",
	    "/var/run/devd.pipe",
	};

	for (const auto* path: pipePaths) {
		if (!QFile::exists(path)) {
			qCInfo(logOSS) << "devd pipe" << path << "does not exist, trying next";
			continue;
		}

		this->mDevdSocket = socket(PF_UNIX, SOCK_SEQPACKET, 0);
		if (this->mDevdSocket < 0) {
			this->mDevdSocket = socket(PF_UNIX, SOCK_STREAM, 0);
		}

		if (this->mDevdSocket < 0) {
			qCWarning(logOSS) << "Failed to create socket for devd:" << qt_error_string(errno);
			continue;
		}

		struct sockaddr_un addr = {};
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

		if (::connect(this->mDevdSocket, (struct sockaddr*) &addr, SUN_LEN(&addr)) == 0) {
			int flags = fcntl(this->mDevdSocket, F_GETFL, 0);
			fcntl(this->mDevdSocket, F_SETFL, flags | O_NONBLOCK);

			this->mDevdNotifier = new QSocketNotifier(this->mDevdSocket, QSocketNotifier::Read, this);
			connect(this->mDevdNotifier, &QSocketNotifier::activated, this, &OSS::handleDevdEvent);

			this->mDevdNotifier->setEnabled(true);
			qCInfo(logOSS) << "Successfully connected to devd at" << path;

			return;
		} else {
			qCWarning(logOSS) << "Failed to connect to" << path << ":" << qt_error_string(errno);
		}

		close(this->mDevdSocket);
		this->mDevdSocket = -1;
	}

	qCWarning(logOSS) << "Failed to connect to a devd pipe—device monitoring disabled";
	// clang-format off
	#else
	qCInfo(logOSS) << "devd monitoring only available on FreeBSD";
	#endif
	// clang-format on
	(void) this;
}

void OSS::handleDevdEvent() {
	std::array<char, 4096> buffer {};
	const ssize_t n = read(this->mDevdSocket, buffer.data(), buffer.size() - 1);

	if (n <= 0) {
		if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			qCWarning(logOSS) << "devd socket read error:" << qt_error_string(errno);
		}
		return;
	}

	buffer[n] = '\0';

	const std::array<const char*, 4> audioKeywords = {"dsp", "mixer", "pcm", "snd"};
	bool isAudioRelated = false;

	for (const auto* keyword: audioKeywords) {
		if (strstr(buffer.data(), keyword) != nullptr) {
			isAudioRelated = true;
			break;
		}
	}

	if (!isAudioRelated) {
		return; // Skip non-audio events
	}

	const QString event = QString::fromUtf8(buffer.data());

	bool needsRescan = false;

	if (event.contains("system=DEVFS")
	    && (event.contains("cdev=dsp") || event.contains("cdev=mixer") || event.contains("cdev=snd")
	        || event.contains("cdev=pcm")))
	{

		qCInfo(logOSS) << "Audio device event detected, scheduling rescan";
		needsRescan = true;
	}

	if (event.contains("hw.snd.default_unit")) {
		qCInfo(logOSS) << "Default device changed, scheduling update";
		needsRescan = true;
	}

	// Prevent multiple rapid rescans when devices are changing quickly
	if (needsRescan && !this->mRescanTimer->isActive()) {
		this->mRescanTimer->start();
	}
}

#ifdef __FreeBSD__
void OSS::handleKqueueEvent() {
	struct kevent event;
	struct timespec timeout = {.tv_sec = 0, .tv_nsec = 0};

	auto nev = kevent(this->mKqueue, nullptr, 0, &event, 1, &timeout);

	if (nev > 0 && (event.fflags & (NOTE_WRITE | NOTE_EXTEND))) {
		this->readNewLogLines();
	}
}

void OSS::setupJackDetection() {
	this->mLogFileDescriptor = open("/var/log/messages", O_RDONLY);
	if (this->mLogFileDescriptor < 0) {
		qCWarning(logOSS) << "Failed to open /var/log/messages for jack detection";
		return;
	}

	// Seek to end to only catch new messages
	this->mLogFilePosition = lseek(this->mLogFileDescriptor, 0, SEEK_END); // NOLINT

	this->mKqueue = kqueue(); // NOLINT
	if (this->mKqueue < 0) {
		qCWarning(logOSS) << "Failed to create kqueue for jack detection";
		close(this->mLogFileDescriptor);
		this->mLogFileDescriptor = -1;
		return;
	}

	struct kevent change;
	EV_SET(
	    &change,
	    this->mLogFileDescriptor,
	    EVFILT_VNODE,
	    EV_ADD | EV_ENABLE | EV_CLEAR,
	    NOTE_WRITE | NOTE_EXTEND,
	    0,
	    nullptr
	);

	if (kevent(this->mKqueue, &change, 1, nullptr, 0, nullptr) < 0) {
		qCWarning(logOSS) << "Failed to register kqueue event:" << qt_error_string(errno);
		close(this->mKqueue);
		close(this->mLogFileDescriptor);

		this->mKqueue = -1;
		this->mLogFileDescriptor = -1;
		return;
	}

	this->mKqueueNotifier = new QSocketNotifier(this->mKqueue, QSocketNotifier::Read, this);
	connect(this->mKqueueNotifier, &QSocketNotifier::activated, this, &OSS::handleKqueueEvent);
	this->mKqueueNotifier->setEnabled(true);

	qCInfo(logOSS) << "Jack detection monitoring enabled";
}
#endif

void OSS::identifyJackType(int nid, bool connected) {
	// Read sysctl to identify what this nid is
	const QString sysctlPath = QString("dev.hdaa.0.nid%1").arg(nid);

	QProcess process;
	process.start("sysctl", QStringList() << "-n" << sysctlPath);
	process.waitForFinished(100);

	const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

	if (output.contains("Headphones", Qt::CaseInsensitive)) {
		qCInfo(logOSS) << "Headphones" << (connected ? "connected" : "disconnected");
		emit this->headphonesChanged(connected);
	} else if (output.contains("Mic", Qt::CaseInsensitive)) {
		qCInfo(logOSS) << "Microphone" << (connected ? "connected" : "disconnected");
		emit this->microphoneChanged(connected);
	}
}

void OSS::syncJackProperties() {
	for (auto it = this->mJackStates.constBegin(); it != this->mJackStates.constEnd(); ++it) {
		const int nid = it.key();
		const bool connected = it.value();

		// Identify what type of jack this is and update properties
		const QString sysctlPath = QString("dev.hdaa.0.nid%1").arg(nid);

		QProcess process;
		process.start("sysctl", QStringList() << "-n" << sysctlPath);
		process.waitForFinished(100);

		const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

		if (output.contains("Headphones", Qt::CaseInsensitive)) {
			if (this->mHeadphonesConnected != connected) {
				this->mHeadphonesConnected = connected;
				qCInfo(logOSS) << "Initial headphone state:" << (connected ? "connected" : "disconnected");
				emit this->headphonesChanged(connected);
			}
		}
	}
}

void OSS::initializeJackStates() {
	QProcess sysctl;
	sysctl.start("sysctl", QStringList() << "-a");
	if (!sysctl.waitForFinished(500)) {
		qCWarning(logOSS) << "sysctl timeout";
		return;
	}

	const QString output = QString::fromUtf8(sysctl.readAllStandardOutput());
	const QStringList lines = output.split('\n');

	for (const QString& line: lines) {
		// Look for jack pins
		if (line.contains("dev.hdaa.") && line.contains(": pin:") && line.contains("Jack")) {
			const QRegularExpression regex(R"(dev\.hdaa\.\d+\.nid(\d+):\s+pin:\s+(.+))");
			auto match = regex.match(line);

			if (match.hasMatch()) {
				const int nid = match.captured(1).toInt();
				const QString pinDesc = match.captured(2);

				// Skip disabled or internal (Fixed/None) pins
				if (pinDesc.contains("DISABLED") || pinDesc.contains("(None)")
				    || pinDesc.contains("(Fixed)"))
				{
					continue;
				}

				qCDebug(logOSS) << "Found jack: nid=" << nid << "desc=" << pinDesc;

				// Initialize as disconnected
				this->mJackStates[nid] = false;

				if (pinDesc.contains("Headphones", Qt::CaseInsensitive)) {
					this->mHeadphonesConnected = false;
					qCInfo(logOSS) << "Found headphone jack at nid" << nid;
				}
			}
		}
	}

	qCDebug(logOSS) << "Initialized" << this->mJackStates.size() << "jacks";

	this->readRecentLogMessages();
	this->syncJackProperties();
}

void OSS::readNewLogLines() {
	QFile logFile;
	if (!logFile.open(
	        this->mLogFileDescriptor,
	        QIODevice::ReadOnly | QIODevice::Text,
	        QFileDevice::DontCloseHandle
	    ))
	{
		return;
	}

	logFile.seek(this->mLogFilePosition);

	QTextStream in(&logFile);
	while (!in.atEnd()) {
		const QString line = in.readLine();
		this->parseLogLine(line);
	}

	// Update position for the next read
	this->mLogFilePosition = logFile.pos();
}

void OSS::parseLogLine(const QString& line) {
	if (!line.contains("Pin sense:")) {
		return;
	}

	// Extract nid number
	const QRegularExpression nidRegex(R"(nid=(\d+))");
	auto nidMatch = nidRegex.match(line);

	if (!nidMatch.hasMatch()) {
		return;
	}

	const int nid = nidMatch.captured(1).toInt();

	bool connected = false;
	if (line.contains("(connected)")) {
		connected = true;
	} else if (line.contains("(disconnected)")) {
		connected = false;
	} else {
		return; // Unknown state
	}

	if (this->mJackStates.value(nid, !connected) != connected) {
		this->mJackStates[nid] = connected;

		qCInfo(logOSS) << "Jack state changed: nid=" << nid
		               << (connected ? "connected" : "disconnected");

		emit this->jackStateChanged(nid, connected);
		this->identifyJackType(nid, connected);
	}
}

void OSS::readRecentLogMessages() {
	QFile logFile("/var/log/messages");
	if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qCWarning(logOSS) << "Cannot read /var/log/messages for initial state";
		return;
	}

	// Read last 8KB for recent events
	const qint64 size = logFile.size();
	if (size > 8192) {
		logFile.seek(size - 8192);
	}

	QTextStream in(&logFile);
	const QString content = in.readAll();
	logFile.close();

	const QStringList lines = content.split('\n');
	QHash<int, bool> lastLoggedState;

	for (const QString& line: lines) {
		if (!line.contains("Pin sense:")) {
			continue;
		}

		const QRegularExpression nidRegex(R"(nid=(\d+))");
		auto nidMatch = nidRegex.match(line);

		if (!nidMatch.hasMatch()) {
			continue;
		}

		const int nid = nidMatch.captured(1).toInt();
		const bool connected = line.contains("(connected)");

		lastLoggedState[nid] = connected;
	}

	for (auto it = lastLoggedState.constBegin(); it != lastLoggedState.constEnd(); ++it) {
		const int nid = it.key();
		const bool connected = it.value();

		if (this->mJackStates.contains(nid)) {
			this->mJackStates[nid] = connected;
		}
	}

	qCDebug(logOSS) << "After reading log, jack states:" << this->mJackStates;
}

bool OSS::headphonesConnected() const { return this->mHeadphonesConnected; }

void OSS::scanDevices() {
	if (this->mDefaultDevice != nullptr) {
		this->mDefaultDevice = nullptr;
		emit this->defaultDeviceChanged();
	}

	qDeleteAll(this->mDevices);
	this->mDevices.clear();

	for (auto i = 0; i < 32; i++) {
		auto mixerPath = QString("/dev/mixer%1").arg(i);

		if (QFile::exists(mixerPath)) {
			auto* device = new OSSSoundDevice(i, this);

			if (device->initialize()) {
				this->mDevices.append(device);
				qCInfo(logOSS) << "Device added:" << device;
			} else {
				qCWarning(logOSS) << "Failed to initialize device:" << device;
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
		qCWarning(logOSS) << "Failed to read default device via /dev/sndstat";
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
	const size_t size = sizeof(deviceId);

	// clang-format off
	#ifdef __FreeBSD__
	if (sysctlbyname("hw.snd.default_unit", nullptr, nullptr, &deviceId, size) == 0) {
		this->updateDefault();
		qCInfo(logOSS) << "Set default device to" << deviceId;
		return true;
	}
	#else
	(void)this;
	(void)size;

	if (!this->mAvailable) {
		qCWarning(logOSS) << "OSS is not available on this system";
		return false;
	}
	#endif
	// clang-format on

	qCWarning(logOSS) << "Failed to set default device to" << deviceId
	                  << "error:" << qt_error_string(errno);

	return false;
}

void OSS::refresh() {
	this->updateDefault();

	for (auto* device: this->mDevices) {
		device->refresh();
	}
}

} // namespace qs::service::oss
