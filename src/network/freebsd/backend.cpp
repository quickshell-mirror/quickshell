#include "backend.hpp"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <utility>

#include <QDebugStateSaver>
#include <QFile>
#include <QIODevice>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QObject>
#include <QOverload>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QSocketNotifier>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <Qt>
#include <QtGlobal>
#include <QtLogging>
#include <QtMinMax>
#include <QtNumeric>
#include <QtTypes>
#include <fcntl.h>
#ifdef __FreeBSD__
#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <sys/event.h>
#include <sys/param.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "../../core/logcat.hpp"
#include "../device.hpp"
#include "../network.hpp"
#include "../wifi.hpp"

namespace {
bool isWiredInterface(const QString& ifname) {
	// Handle virtual interfaces too
	return ifname.startsWith("eth") ||   // Generic
	       ifname.startsWith("tun") ||   // TUN
	       ifname.startsWith("tap") ||   // TAP
	       ifname.startsWith("wg") ||    // WireGuard
	       ifname.startsWith("ppp") ||   // Point-to-Point
	       ifname.startsWith("ue") ||    // USB Ethernet interface
	       ifname.startsWith("em") ||    // Intel PRO/1000
	       ifname.startsWith("igb") ||   // Intel I350/I210/I211
	       ifname.startsWith("ix") ||    // Intel 10Gb
	       ifname.startsWith("ixl") ||   // Intel XL710
	       ifname.startsWith("re") ||    // RealTek
	       ifname.startsWith("rl") ||    // RealTek 8129/8139
	       ifname.startsWith("bge") ||   // Broadcom BCM57xx/BCM590x
	       ifname.startsWith("bce") ||   // QLogic NetXtreme II
	       ifname.startsWith("fxp") ||   // Intel EtherExpress PRO/100
	       ifname.startsWith("dc") ||    // DEC/Intel 21143
	       ifname.startsWith("xl") ||    // 3Com Etherlink XL, Fast Etherlink XL
	       ifname.startsWith("vr") ||    // VIA Rhine I/II/III
	       ifname.startsWith("sis") ||   // SiS 900/7016 900, NS DP83815
	       ifname.startsWith("sk") ||    // SysKonnect SK-984x/SK-982x
	       ifname.startsWith("ste") ||   // Sundance ST201
	       ifname.startsWith("age") ||   // Attansic/Atheros L1
	       ifname.startsWith("ale") ||   // Atheros AR8121/AR8113/AR8114
	       ifname.startsWith("alc") ||   // Atheros AR813x/AR815x/AR816x/AR817x
	       ifname.startsWith("ae") ||    // Attansic/Atheros L2
	       ifname.startsWith("axe") ||   // ASIX Electronics AX88x7x/760
	       ifname.startsWith("cxgbe") || // Chelsio T4/T5/T6
	       ifname.startsWith("mlx");     // Mellanox ConnectX-3
}

bool isWirelessInterface(const QString& ifname) { return ifname.startsWith("wlan"); }
bool isIgnoredInterface(const QString& ifname) {
	return ifname.startsWith("lo") || ifname.startsWith("pflog") || ifname.startsWith("pfsync")
	    || ifname.startsWith("bastille") || ifname.startsWith("bridge") || ifname.startsWith("gif")
	    || ifname.startsWith("gre") || ifname.startsWith("stf");
}
} // namespace

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkFreeBSD, "quickshell.network.freebsd", QtWarningMsg);
}

// FreeBSDBackend implementation
FreeBSDBackend::FreeBSDBackend(QObject* parent)
    : NetworkBackend(parent)
    , bWifiEnabled(true)
    , bWifiHardwareEnabled(true) {

	this->initializeRouteSocket();
	this->initializeDevdSocket();

	// Defer device scan until after signals are connected
	QMetaObject::invokeMethod(this, &FreeBSDBackend::scanExistingDevices, Qt::QueuedConnection);
}

FreeBSDBackend::~FreeBSDBackend() {
	for (auto* process: this->mPendingProcesses) {
		if (process->state() != QProcess::NotRunning) {
			process->kill();
			process->waitForFinished(1000);
		}

		delete process;
	}

	this->cleanupSockets();
	this->mPendingProcesses.clear();
}

bool FreeBSDBackend::isAvailable() const {
	// clang-format off
	#ifdef __FreeBSD__
	// clang-format on
	bool available = QFile::exists("/sbin/ifconfig");
	if (!available) {
		qCDebug(logNetworkFreeBSD) << "ifconfig not found";
		qCWarning(logNetworkFreeBSD) << "FreeBSD network backend is not available";
	}

	return available;
	// clang-format off
	#else
	qCDebug(logNetworkFreeBSD) << "Not compiled for FreeBSD";
	return false;
	#endif
	// clang-format on
}

void FreeBSDBackend::initializeRouteSocket() {
	qCDebug(logNetworkFreeBSD) << "Connecting to the route socket";
	this->mRouteSocket = socket(PF_ROUTE, SOCK_RAW, 0);
	if (this->mRouteSocket < 0) {
		qCWarning(logNetworkFreeBSD) << "Failed to connect to the route socket:"
		                             << qt_error_string(errno);
		return;
	}

	const int flags = fcntl(this->mRouteSocket, F_GETFL, 0);
	fcntl(this->mRouteSocket, F_SETFL, flags | O_NONBLOCK);

	this->mRouteNotifier = new QSocketNotifier(this->mRouteSocket, QSocketNotifier::Read, this);
	QObject::connect(
	    this->mRouteNotifier,
	    &QSocketNotifier::activated,
	    this,
	    &FreeBSDBackend::onRouteSocketActivated
	);

	qCInfo(logNetworkFreeBSD) << "Route socket initialized for interface events";
}

// NOLINTNEXTLINE
void FreeBSDBackend::initializeDevdSocket() {
	const std::array pipePaths = {
	    "/var/run/devd.seqpacket.pipe",
	    "/var/run/devd.pipe",
	};

	for (const auto* path: pipePaths) {
		if (!QFile::exists(path)) {
			qCDebug(logNetworkFreeBSD) << "devd pipe" << path << "does not exist";
			continue;
		}

		if (path == pipePaths[0]) {
			qCDebug(logNetworkFreeBSD) << "Connecting to SOCK_SEQPACKET";
			this->mDevdFd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
		} else {
			qCDebug(logNetworkFreeBSD) << "Falling back to SOCK_STREAM";
			this->mDevdFd = socket(PF_UNIX, SOCK_STREAM, 0);
		}

		if (this->mDevdFd < 0) {
			qCWarning(logNetworkFreeBSD)
			    << "Failed to connect a socket for devd:" << qt_error_string(errno);
			continue;
		}

		struct sockaddr_un addr = {};
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

		if (::connect(this->mDevdFd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr)) == 0) {
			const int flags = fcntl(this->mDevdFd, F_GETFL, 0);
			fcntl(this->mDevdFd, F_SETFL, flags | O_NONBLOCK);

			this->mDevdNotifier = new QSocketNotifier(this->mDevdFd, QSocketNotifier::Read, this);
			QObject::connect(
			    this->mDevdNotifier,
			    &QSocketNotifier::activated,
			    this,
			    &FreeBSDBackend::onDevdActivated
			);

			this->mDevdNotifier->setEnabled(true);
			qCInfo(logNetworkFreeBSD) << "Successfully connected to" << path;
			return;
		} else {
			qCWarning(logNetworkFreeBSD)
			    << "Failed to connect to" << path << ":" << qt_error_string(errno);
			close(this->mDevdFd);
			this->mDevdFd = -1;
		}
	}
}

void FreeBSDBackend::cleanupSockets() {
	if (this->mRouteNotifier) {
		delete this->mRouteNotifier;
		this->mRouteNotifier = nullptr;
	}

	if (this->mRouteSocket >= 0) {
		close(this->mRouteSocket);
		this->mRouteSocket = -1;
	}

	if (this->mDevdNotifier) {
		delete this->mDevdNotifier;
		this->mDevdNotifier = nullptr;
	}

	if (this->mDevdFd >= 0) {
		close(this->mDevdFd);
		this->mDevdFd = -1;
	}
}

void FreeBSDBackend::onRouteSocketActivated() {
	std::array<char, 2048> buf {};
	ssize_t n = 0;

	while ((n = read(this->mRouteSocket, buf.data(), buf.size())) > 0) {
		this->handleRouteMessage(buf.data(), n);
	}
}

void FreeBSDBackend::handleRouteMessage(const char* buf, ssize_t len) {
	// clang-format off
	#ifdef __FreeBSD__
	// clang-format on
	if (len < static_cast<ssize_t>(sizeof(struct rt_msghdr))) return;

	const auto* rtm = reinterpret_cast<const struct rt_msghdr*>(buf);

	// We are interested in interface announcements only
	if (rtm->rtm_type == RTM_IFANNOUNCE) {
		if (len < static_cast<ssize_t>(sizeof(struct if_announcemsghdr))) {
			qCWarning(logNetworkFreeBSD) << "RTM_IFANNOUNCE message is too small";
			return;
		}

		const auto* ifan = reinterpret_cast<const struct if_announcemsghdr*>(buf);
		QString ifname = QString::fromLatin1(ifan->ifan_name);

		if (isIgnoredInterface(ifname)) {
			qCDebug(logNetworkFreeBSD) << "Ignoring interface:" << ifname;
			return;
		}

		if (ifan->ifan_what == IFAN_ARRIVAL) {
			qCInfo(logNetworkFreeBSD) << "Interface arrived:" << ifname;
			this->processInterface(ifname, true);
		} else if (ifan->ifan_what == IFAN_DEPARTURE) {
			qCInfo(logNetworkFreeBSD) << "Interface departed:" << ifname;
			this->removeInterface(ifname);
		}
	}

	// RTM_IFINFO messages indicate interface state changes
	else if (rtm->rtm_type == RTM_IFINFO)
	{
		if (len < static_cast<ssize_t>(sizeof(struct if_msghdr))) {
			qCWarning(logNetworkFreeBSD) << "RTM_IFINFO message too small";
			return;
		}

		const auto* ifm = reinterpret_cast<const struct if_msghdr*>(buf);
		std::array<char, IF_NAMESIZE> ifnameBuf {};

		if (if_indextoname(ifm->ifm_index, ifnameBuf.data())) {
			const QString ifname = QString::fromLatin1(ifnameBuf);

			if (!isIgnoredInterface(ifname) && this->mDevices.contains(ifname)) {
				qCDebug(logNetworkFreeBSD) << "Interface state changed:" << ifname;

				if (auto* wifiDev = qobject_cast<FreeBSDWifiDevice*>(this->mDevices[ifname])) {
					wifiDev->handleInterfaceEvent();
				} else if (auto* wiredDev = qobject_cast<FreeBSDWiredDevice*>(this->mDevices[ifname])) {
					wiredDev->handleInterfaceEvent();
				}
			}
		}
	}
	// clang-format off
	#else
	(void)this;
	(void)buf;
	(void)len;
	qCWarning(logNetworkFreeBSD) << "Requires FreeBSD";
	#endif
	// clang-format off
}

void FreeBSDBackend::onDevdActivated() {
	std::array<char, 4096> buf {};
	ssize_t n = 0;

	while ((n = read(this->mDevdFd, buf.data(), buf.size() - 1)) > 0) {
		buf[n] = '\0';
		this->mDevdBuffer.append(buf.data(), n);
	}

	while (true) {
		const qsizetype newlineIdx = this->mDevdBuffer.indexOf('\n');
		if (newlineIdx == -1) break;

		if (newlineIdx < 0 || newlineIdx >= this->mDevdBuffer.size()) {
			qCWarning(logNetworkFreeBSD) << "Invalid newline index";
			break;
		}

		QByteArray line = this->mDevdBuffer.left(newlineIdx);
		this->mDevdBuffer.remove(0, newlineIdx + 1);

		if (!line.isEmpty() && line[0] == '!') {
			this->handleDevdEvent(QString::fromUtf8(line));
		}
	}
}

void FreeBSDBackend::handleDevdEvent(const QString& event) {
	// Looking for IFNET events
	if (!event.contains("system=IFNET")) return;

	const QRegularExpression re(R"(subsystem=(\w+)\s+type=(\w+))");
	auto match = re.match(event);

	if (match.hasMatch()) {
		const QString subsys = match.captured(1);
		const QString type = match.captured(2);

		if (isIgnoredInterface(subsys)) {
			qCDebug(logNetworkFreeBSD) << "Ignoring devd event for interface:" << subsys;
			return;
		}

		if (type == "ATTACH") {
			qCInfo(logNetworkFreeBSD) << "devd: Interface attached:" << subsys;
			this->processInterface(subsys, true);
		} else if (type == "DETACH") {
			qCInfo(logNetworkFreeBSD) << "devd: Interface detached:" << subsys;
			this->removeInterface(subsys);
		} else if (this->mDevices.contains(subsys)) {
			// Handle other interface events (LINK_UP, LINK_DOWN, etc.)
			qCDebug(logNetworkFreeBSD) << "devd: Interface event:" << subsys << type;

			if (auto* wifiDev = qobject_cast<FreeBSDWifiDevice*>(this->mDevices[subsys])) {
				wifiDev->handleInterfaceEvent();
			} else if (auto* wiredDev = qobject_cast<FreeBSDWiredDevice*>(this->mDevices[subsys])) {
				wiredDev->handleInterfaceEvent();
			}
		}
	}
}

void FreeBSDBackend::scanExistingDevices() {
	auto* process = new QProcess();
	this->mPendingProcesses.append(process);

	QObject::connect(
		process,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, process](int exitCode, QProcess::ExitStatus) {
			if (exitCode != 0) {
				qCWarning(logNetworkFreeBSD) << "Failed to list network interfaces";
				process->deleteLater();
				return;
			}

			const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
			const auto interfaces = output.split(' ', Qt::SkipEmptyParts);

			qCInfo(logNetworkFreeBSD) << "Scanning existing interfaces:" << interfaces;

			for (const QString& iface: interfaces) {
				if (isIgnoredInterface(iface)) {
					qCDebug(logNetworkFreeBSD) << "Skipping ignored interface:" << iface;
					continue;
				}

				qCInfo(logNetworkFreeBSD) << "Processing interface:" << iface;
				this->processInterface(iface, true);
			}

			qCInfo(logNetworkFreeBSD) << "Total device count is" << this->mDevices.size();

			this->mPendingProcesses.removeOne(process);
			process->deleteLater();
		}
	);

	process->start("ifconfig", QStringList() << "-l");
}

void FreeBSDBackend::processInterface(const QString& interfaceName, bool isNew) {
	if (this->mDevices.contains(interfaceName)) {
		if (!isNew) {
			qCDebug(logNetworkFreeBSD) << "Updating existing device:" << interfaceName;

			if (auto* wifiDev = qobject_cast<FreeBSDWifiDevice*>(this->mDevices[interfaceName])) {
				wifiDev->handleInterfaceEvent();
			} else if (auto* wiredDev = qobject_cast<FreeBSDWiredDevice*>(this->mDevices[interfaceName]))
			{
				wiredDev->handleInterfaceEvent();
			}
		} else {
			qCDebug(logNetworkFreeBSD) << "Device already exists:" << interfaceName;
		}

		return;
	}

	if (isWirelessInterface(interfaceName)) {
		qCInfo(logNetworkFreeBSD) << "New wireless device:" << interfaceName;
		auto* device = new FreeBSDWifiDevice(interfaceName, this);

		this->mDevices.insert(interfaceName, device);
		emit this->deviceAdded(device); // NOLINT

		qCDebug(logNetworkFreeBSD) << "Device pointer:" << device;
	} else if (isWiredInterface(interfaceName)) {
		qCInfo(logNetworkFreeBSD) << "New wired device:" << interfaceName;
		auto* device = new FreeBSDWiredDevice(interfaceName, this);

		this->mDevices.insert(interfaceName, device);
		emit this->deviceAdded(device); // NOLINT

		qCDebug(logNetworkFreeBSD) << "Device pointer:" << device;
	} else {
		qCDebug(logNetworkFreeBSD) << "Skipping unknown interface type" << interfaceName;
	}
}

void FreeBSDBackend::removeInterface(const QString& interfaceName) {
	if (!this->mDevices.contains(interfaceName)) return;

	qCInfo(logNetworkFreeBSD) << "Removing device" << interfaceName;
	auto* device = this->mDevices.take(interfaceName);
	emit this->deviceRemoved(device);
	device->deleteLater();
}

void FreeBSDBackend::setWifiEnabled(bool enabled) {
	if (this->bWifiEnabled == enabled) return;

	for (auto* device: this->mDevices) {
		auto* process = new QProcess(this);
		this->mPendingProcesses.append(process);

		QObject::connect(
			process,
			QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this,
			[this, process](int, QProcess::ExitStatus) {
				this->mPendingProcesses.removeOne(process);
				process->deleteLater();
			}
		);

		process->start("ifconfig", QStringList() << device->name() << (enabled ? "up" : "down"));
	}

	this->bWifiEnabled = enabled;
	emit this->wifiEnabledChanged();
}

// FreeBSDWiredDevice implementation
FreeBSDWiredDevice::FreeBSDWiredDevice(const QString& interfaceName, QObject* parent)
    : NetworkDevice(DeviceType::None, parent)
    , mInterfaceName(interfaceName) {

	qCInfo(logNetworkFreeBSD) << "Creating wired device for" << interfaceName;

	this->updateName(interfaceName);

	QObject::connect(
	    this,
	    &NetworkDevice::requestDisconnect,
	    this,
	    &FreeBSDWiredDevice::onDisconnectRequested
	);

	QObject::connect(
	    this,
	    &NetworkDevice::requestSetAutoconnect,
	    this,
	    &FreeBSDWiredDevice::onSetAutoconnectRequested
	);

	this->updateStateFromIfconfig();
}

FreeBSDWiredDevice::~FreeBSDWiredDevice() {
	for (auto* process : this->mPendingProcesses) {
		if (process->state() != QProcess::NotRunning) {
			process->kill();
			process->waitForFinished(1000);
		}

		delete process;
	}

	this->mPendingProcesses.clear();
}

void FreeBSDWiredDevice::updateName(const QString& name) { this->bindableName().setValue(name); }

void FreeBSDWiredDevice::updateAddress(const QString& address) {
	this->bindableAddress().setValue(address);
}

void FreeBSDWiredDevice::updateConnectionState(DeviceConnectionState::Enum state) {
	this->bindableState().setValue(state);
	this->bindableConnected().setValue(state == DeviceConnectionState::Connected);
}

void FreeBSDWiredDevice::handleInterfaceEvent() {
	qCDebug(logNetworkFreeBSD) << "Handling interface event for" << this->mInterfaceName;
	this->updateStateFromIfconfig();
}

void FreeBSDWiredDevice::updateStateFromIfconfig() {
	auto* process = new QProcess(this);
	this->mPendingProcesses.append(process);

	QObject::connect(
		process,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, process](int exitCode, QProcess::ExitStatus) {
			if (exitCode == 0) {
				const QString output = QString::fromUtf8(process->readAllStandardOutput());
				this->parseIfconfigOutput(output);
			}

			this->mPendingProcesses.removeOne(process);
			process->deleteLater();
		}
	);

	process->start("ifconfig", QStringList() << this->mInterfaceName);
}

void FreeBSDWiredDevice::parseIfconfigOutput(const QString& output) {
	const QRegularExpression macRegex(R"(ether\s+([0-9a-f:]{17}))");
	auto match = macRegex.match(output);

	if (match.hasMatch()) {
		this->updateAddress(match.captured(1));
	}

	// Check if an interface is UP and has link/carrier
	const bool isUp = output.contains(QRegularExpression("\\bUP\\b"));
	const bool hasCarrier = !output.contains("status: no carrier");
	const bool isActive = output.contains("status: active");
	// Consider connected when an interface is UP and either has carrier or shows active
	const bool isConnected = isUp && (hasCarrier || isActive);

	this->updateConnectionState(
	    isConnected ? DeviceConnectionState::Connected : DeviceConnectionState::Disconnected
	);
}

void FreeBSDWiredDevice::onDisconnectRequested() {
	qCInfo(logNetworkFreeBSD) << "Disconnecting" << this->mInterfaceName;

	this->updateConnectionState(DeviceConnectionState::Disconnecting);

	auto* process = new QProcess(this);
	this->mPendingProcesses.append(process);

	QObject::connect(
		process,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, process](int, QProcess::ExitStatus) {
			this->updateStateFromIfconfig();
			this->mPendingProcesses.removeOne(process);
			process->deleteLater();
		}
	);

	process->start("ifconfig", QStringList() << this->mInterfaceName << "down");
}

void FreeBSDWiredDevice::onSetAutoconnectRequested(bool autoconnect) {
	this->mAutoconnect = autoconnect;
	qCInfo(logNetworkFreeBSD) << "Autoconnect for" << this->mInterfaceName << "set to" << autoconnect;
	this->bindableAutoconnect().setValue(autoconnect);
}

// FreeBSDWifiDevice implementation
FreeBSDWifiDevice::FreeBSDWifiDevice(const QString& interfaceName, QObject* parent)
    : WifiDevice(parent)
    , mInterfaceName(interfaceName) {

	qCInfo(logNetworkFreeBSD) << "Creating wireless device for" << interfaceName;

	this->updateName(interfaceName);

	QProcess process;
	process.start("ifconfig", QStringList() << interfaceName);
	if (process.waitForFinished(3000)) {
		const QString output = QString::fromUtf8(process.readAllStandardOutput());

		// Extract BSSID
		const QRegularExpression bssidRegex(R"(bssid\s+([0-9a-f:]{17}))", QRegularExpression::CaseInsensitiveOption);
		auto match = bssidRegex.match(output);

		if (match.hasMatch()) {
			this->mCurrentBssid = match.captured(1).toLower();
		}

		// Extract SSID
		const QRegularExpression ssidRegex(R"regex(ssid\s+(?:"([^"]+)"|(\S+)))regex");
		match = ssidRegex.match(output);

		if (match.hasMatch()) {
			this->mCurrentSsid = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
		}

		// Parse full state
		this->parseIfconfigOutput(output);
	}

	QObject::connect(
	    this,
	    &NetworkDevice::requestDisconnect,
	    this,
	    &FreeBSDWifiDevice::onDisconnectRequested
	);

	QObject::connect(
	    this,
	    &NetworkDevice::requestSetAutoconnect,
	    this,
	    &FreeBSDWifiDevice::onSetAutoconnectRequested
	);

	QObject::connect(this, &WifiDevice::scannerEnabledChanged, this, [this](bool enabled) {
		if (enabled) this->triggerScan();
	});

}

FreeBSDWifiDevice::~FreeBSDWifiDevice() {
	for (auto* process : this->mPendingProcesses) {
		if (process->state() != QProcess::NotRunning) {
			process->kill();
			process->waitForFinished(1000);
		}

		delete process;
	}

	for (auto* thread : this->mPendingThreads) {
		if (thread->isRunning()) {
			thread->quit();
			thread->wait(1000);
		}

		delete thread;
	}

	this->mPendingProcesses.clear();
	this->mPendingThreads.clear();
}

void FreeBSDWifiDevice::updateName(const QString& name) { this->bindableName().setValue(name); }

void FreeBSDWifiDevice::updateAddress(const QString& address) {
	this->bindableAddress().setValue(address);
}

void FreeBSDWifiDevice::updateConnectionState(DeviceConnectionState::Enum state) {
	this->bindableState().setValue(state);
	this->bindableConnected().setValue(state == DeviceConnectionState::Connected);
}

void FreeBSDWifiDevice::updateMode(WifiDeviceMode::Enum mode) {
	this->bindableMode().setValue(mode);
}

void FreeBSDWifiDevice::loadKnownNetworks() {
	auto* thread = new QThread();
	auto* worker = new QObject();
	worker->moveToThread(thread);
	this->mPendingThreads.append(thread);

	const QString ifaceName = this->mInterfaceName;

	QObject::connect(thread, &QThread::started, worker, [worker, ifaceName]() {
		QSet<QString> knownNetworks;
		const QStringList configPaths = {
		    "/etc/wpa_supplicant.conf",
		    "/usr/local/etc/wpa_supplicant.conf"
		};

		for (const QString& configPath: configPaths) {
			QFile configFile(configPath);
			if (!configFile.exists()) {
				qCDebug(logNetworkFreeBSD) << "Config file does not exist:" << configPath;
				continue;
			}

			if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
				qCWarning(logNetworkFreeBSD) << "Failed to open config file:" << configPath;
				continue;
			}

			qCDebug(logNetworkFreeBSD) << "Parsing config:" << configPath;
			QTextStream in(&configFile);
			bool inNetworkBlock = false;
			QString currentSsid;

			while (!in.atEnd()) {
				const QString line = in.readLine().trimmed();

				if (line.startsWith('#')) continue;
				if (line == "network={") {
					inNetworkBlock = true;
					currentSsid.clear();
				} else if (line == "}" && inNetworkBlock) {
					if (!currentSsid.isEmpty()) {
						knownNetworks.insert(currentSsid);
						qCDebug(logNetworkFreeBSD) << "Found known network:" << currentSsid;
					}

					inNetworkBlock = false;
				} else if (inNetworkBlock && line.startsWith("ssid=")) {
					QString ssidValue = line.mid(5).trimmed();

					// Handle both quoted and unquoted SSIDs
					if (ssidValue.startsWith('"') && ssidValue.endsWith('"')) {
						if (ssidValue.length() >= 2) {
							ssidValue = ssidValue.mid(1, ssidValue.length() - 2);
							ssidValue.replace(R"(\")", "\"");
							ssidValue.replace(R"(\\)", "\\");
						} else {
							qCWarning(logNetworkFreeBSD) << "Invalid quoted SSID (too short)";
							continue;
						}
					} else if (ssidValue.startsWith("P\"")) {
						qCDebug(logNetworkFreeBSD) << "Skipping P-string SSID format:" << ssidValue;
						continue;
					}

					currentSsid = ssidValue;
				}
			}

			configFile.close();
		}

		// Parse rc.conf for interface-specific SSID configurations
		QFile rcConf("/etc/rc.conf");
		if (rcConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
			qCDebug(logNetworkFreeBSD) << "Parsing /etc/rc.conf";
			QTextStream in(&rcConf);

			// Match patterns like: ifconfig_wlan0="WPA DHCP ssid MyNetwork"
			// or: wlans_iwn0="wlan0"
			const QString ifconfigKey = QString("ifconfig_%1").arg(ifaceName);
			const QRegularExpression ssidRegex(R"regex(ssid\s+(?:"([^"]+)|([^\s]+)))regex");

			while (!in.atEnd()) {
				const QString line = in.readLine().trimmed();

				if (line.startsWith('#')) continue; // Skip comments
				if (line.startsWith(ifconfigKey)) {
					auto match = ssidRegex.match(line);
					if (match.hasMatch()) {
						QString ssid;
						if (match.lastCapturedIndex() >= 1) {
							// captured(1) is quoted SSID, captured(2) is unquoted
							ssid = match.captured(1).isEmpty()
							         ? (match.lastCapturedIndex() >= 2 ? match.captured(2) : QString())
							         : match.captured(1);
						}

						if (!ssid.isEmpty()) {
							knownNetworks.insert(ssid);
							qCDebug(logNetworkFreeBSD) << "Found known network in rc.conf:" << ssid;
						}
					}
				}
			}

			rcConf.close();
		}

		worker->setProperty("knownNetworks", QVariant::fromValue(knownNetworks));
		worker->thread()->quit();
	});

	QObject::connect(thread, &QThread::finished, this, [this, worker, thread]() {
		auto knownNetworks = worker->property("knownNetworks").value<QSet<QString>>();

		// Mark networks as known based on SSID match across all BSSIDs
		for (const QString& ssid: knownNetworks) {
			for (auto* network : this->mNetworkMap) {
				if (network->name() == ssid) {
					network->updateKnown(true);
				}
			}
		}

		this->mPendingThreads.removeOne(thread);
		worker->deleteLater();
		thread->deleteLater();
	});

	thread->start();

	// Query running wpa_supplicant with wpa_cli
	auto* wpaCliProcess = new QProcess(this);
	this->mPendingProcesses.append(wpaCliProcess);

	connect(
		wpaCliProcess,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, wpaCliProcess](int exitCode, QProcess::ExitStatus) {
				if (exitCode == 0) {
				// Skip header line
				const QString output = QString::fromUtf8(wpaCliProcess->readAllStandardOutput());
				QStringList lines = output.split('\n', Qt::SkipEmptyParts);

				for (int i = 1; i < lines.size(); ++i) {
					const QString line = lines[i].trimmed();
					if (line.isEmpty()) continue;

					// Format: network_id / ssid / bssid / flags
					QStringList fields = line.split('\t', Qt::SkipEmptyParts);
					if (fields.size() >= 2) {
						const QString ssid = fields[1].trimmed();
						if (!ssid.isEmpty()) {
							for (auto* network : this->mNetworkMap) {
								if (network->name() == ssid) {
									qCDebug(logNetworkFreeBSD) << "Found known network via wpa_cli:" << ssid;
									network->updateKnown(true);
								}
							}
						}
					}
				}
			}

			this->mPendingProcesses.removeOne(wpaCliProcess);
			wpaCliProcess->deleteLater();
	    }
	);

	wpaCliProcess->start("wpa_cli", QStringList() << "-i" << this->mInterfaceName << "list_networks");
}

void FreeBSDWifiDevice::triggerScan() {
	if (this->mScanPending) {
		qCDebug(logNetworkFreeBSD) << "Scan already pending for" << this->mInterfaceName;
		return;
	}

	qCDebug(logNetworkFreeBSD) << "Triggering scan on" << this->mInterfaceName;
	this->mScanPending = true;

	auto* ifconfigProcess = new QProcess(this);
	this->mPendingProcesses.append(ifconfigProcess);

	QObject::connect(
		ifconfigProcess,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, ifconfigProcess](int exitCode, QProcess::ExitStatus) {
			if (exitCode == 0) {
				const QString output = QString::fromUtf8(ifconfigProcess->readAllStandardOutput());
				const QRegularExpression bssidRegex(R"(bssid\s+([0-9a-f:]{17}))", QRegularExpression::CaseInsensitiveOption);
				auto match = bssidRegex.match(output);

				if (match.hasMatch()) {
					this->mCurrentBssid = match.captured(1).toLower();
				} else {
					this->mCurrentBssid.clear();
				}
			}

			this->mPendingProcesses.removeOne(ifconfigProcess);
			ifconfigProcess->deleteLater();

			// trigger the actual scan
			auto* scanRequestProcess = new QProcess(this);
			this->mPendingProcesses.append(scanRequestProcess);

			QObject::connect(
				scanRequestProcess,
				QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
				this,
				[this, scanRequestProcess](int, QProcess::ExitStatus) {
					this->mPendingProcesses.removeOne(scanRequestProcess);
					scanRequestProcess->deleteLater();

					QTimer::singleShot(1500, this, [this]() {
						auto* scanResultProcess = new QProcess(this);
						this->mPendingProcesses.append(scanResultProcess);

						QObject::connect(
							scanResultProcess,
							QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
							this,
							[this, scanResultProcess](int exitCode, QProcess::ExitStatus) {
								if (exitCode != 0) {
									this->mScanPending = false;
									qCWarning(logNetworkFreeBSD) << "Scan failed for" << this->mInterfaceName;
									this->mPendingProcesses.removeOne(scanResultProcess);
									scanResultProcess->deleteLater();
									return;
								}

								const QString output = QString::fromUtf8(scanResultProcess->readAllStandardOutput());
								this->mPendingProcesses.removeOne(scanResultProcess);
								scanResultProcess->deleteLater();

								const QStringList lines = output.split('\n');
								this->processScanLines(lines, 0);
							}
						);

						scanResultProcess->start(
							"ifconfig",
							QStringList() << this->mInterfaceName << "list" << "scan"
						);
					});
				}
			);

			scanRequestProcess->start("ifconfig", QStringList() << this->mInterfaceName << "scan");
		}
	);

	ifconfigProcess->start("ifconfig", QStringList() << this->mInterfaceName);
}

void FreeBSDWifiDevice::processScanLines(const QStringList& lines, int64_t startIndex) {
	const int chunkSize = 5;
	const auto endIndex = qMin(startIndex + chunkSize, lines.size());
	const bool isCurrentlyConnected =
	    (this->bindableState().value() == DeviceConnectionState::Connected);

	if (startIndex == 0) {
		if (this->mCurrentBssid.isEmpty() && isCurrentlyConnected) {
			qCWarning(logNetworkFreeBSD) << "WARNING: Device is connected but mCurrentBssid is EMPTY!";
			qCWarning(logNetworkFreeBSD) << "This means BSSID was not extracted from ifconfig output!";
		}
	}

	static bool enumValuesPrinted = false;
	if (!enumValuesPrinted) {
		enumValuesPrinted = true;
		qCDebug(logNetworkFreeBSD) << "WifiSecurityType values -"
		                           << "Open:" << static_cast<int>(WifiSecurityType::Open) << "|"
		                           << "StaticWep:" << static_cast<int>(WifiSecurityType::StaticWep)
		                           << "|" << "WpaPsk:" << static_cast<int>(WifiSecurityType::WpaPsk)
		                           << "|" << "Wpa2Psk:" << static_cast<int>(WifiSecurityType::Wpa2Psk)
		                           << "|" << "Sae:" << static_cast<int>(WifiSecurityType::Sae);
	}

	for (int64_t i = startIndex; i < endIndex; ++i) {
		// Skip empty lines and header
		const QString trimmedLine = lines[i].trimmed();
		if (trimmedLine.isEmpty() || trimmedLine.startsWith("SSID")) {
			continue;
		}

		// Match BSSID (MAC address) pattern
		const QRegularExpression bssidRegex(
		    "([0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2})",
		    QRegularExpression::CaseInsensitiveOption
		);

		auto bssidMatch = bssidRegex.match(trimmedLine);
		if (!bssidMatch.hasMatch()) continue;

		const qsizetype bssidPos = bssidMatch.capturedStart(1);
		if (bssidPos < 0 || bssidPos >= trimmedLine.length()) continue;

		// Everything before BSSID is the SSID
		QString ssid = trimmedLine.left(bssidPos).trimmed();
		const QString bssid = bssidMatch.captured(1).toLower();
		const qsizetype afterBssidStart = bssidPos + bssidMatch.captured(1).length();

		if (afterBssidStart > trimmedLine.length()) continue;

		const QString afterBssid = trimmedLine.mid(afterBssidStart).trimmed();
		QStringList fields = afterBssid.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

		if (fields.size() < 5) continue;

		if (ssid.isEmpty()) {
			ssid = "EMPTY SSID";
			//continue;
		}

		// Remove quotes if present
		ssid.remove('"');

		if (fields.size() <= 2) continue;

		// Parse S:N field (signal:noise) - should be at index 2
		const auto& snField = fields[2];
		QStringList snParts = snField.split(':');
		int signal = -100;

		if (snParts.size() >= 2) { // Check size before accessing
			signal = snParts[0].toInt();
		}

		const qreal strength = qBound(0.0, (signal + 100.0) / 80.0, 1.0);

		if (fields.size() <= 4) continue;

		// CAPS field starts at index 4 (after CHAN, RATE, S:N, INT)
		QStringList capsFields;
		for (int cfi = 4; cfi < fields.size(); ++cfi) {
			capsFields.append(fields[cfi]);
		}

		const QString capsString = capsFields.join(" ");
		WifiSecurityType::Enum security = WifiSecurityType::Unknown;

		if (capsString.contains("RSN")) {
			if (capsString.contains("SAE")) {
				security = WifiSecurityType::Sae;
			} else {
				security = WifiSecurityType::Wpa2Psk;
			}
		} else if (capsString.contains("WPA2")) {
			security = WifiSecurityType::Wpa2Psk;
		} else if (capsString.contains("WPA")) {
			security = WifiSecurityType::WpaPsk;
		} else if (capsString.contains("P")) {
			security = WifiSecurityType::StaticWep;
		} else {
			security = WifiSecurityType::Open;
		}

		// Use BSSID as the unique key
		FreeBSDWifiNetwork* network = nullptr;
		bool isNewNetwork = false;

		if (this->mNetworkMap.contains(bssid)) {
			network = this->mNetworkMap[bssid];
		} else {
			network = new FreeBSDWifiNetwork(ssid, bssid, this->mInterfaceName, this);
			isNewNetwork = true;
		}

		network->updateSignalStrength(strength);
		network->updateSecurity(security);

		qCDebug(logNetworkFreeBSD) << "Network:" << ssid << "BSSID:" << bssid
		                           << "| Signal:" << signal << "| Strength:" << qRound(strength * 100)
		                           << "%"
		                           << "| Security:" << static_cast<int>(security)
		                           << "| CAPS:" << capsString;

		// Check if this specific BSSID is currently connected
		const bool isConnectedBssid = (!this->mCurrentBssid.isEmpty() && bssid == this->mCurrentBssid && isCurrentlyConnected);

		if (isConnectedBssid) {
			network->updateConnectionState(NetworkState::Connected);
		} else {
			network->updateConnectionState(NetworkState::Disconnected);
		}

		if (isNewNetwork) {
			this->mNetworkMap.insert(bssid, network);
			this->networkAdded(network);
		}
	}

	if (endIndex < lines.size()) {
		// Process next chunk
		QTimer::singleShot(0, this, [this, lines, endIndex]() {
			this->processScanLines(lines, endIndex);
		});
	} else {
		// Done
		this->mScanPending = false;
		this->loadKnownNetworks();
		qCDebug(logNetworkFreeBSD) << "Scan processing completed for" << this->mInterfaceName;
	}
}

void FreeBSDWifiDevice::onScanTimerTimeout() {
	if (this->scannerEnabled() && !this->mScanPending) {
		this->triggerScan();
	}
}

void FreeBSDWifiDevice::handleInterfaceEvent() {
	qCDebug(logNetworkFreeBSD) << "Handling interface event for" << this->mInterfaceName;
	this->updateStateFromIfconfig();
}

void FreeBSDWifiDevice::handleScanComplete() {
	if (this->scannerEnabled()) {
		this->triggerScan();
	}
}

void FreeBSDWifiDevice::updateStateFromIfconfig() {
	auto* process = new QProcess(this);
	this->mPendingProcesses.append(process);

	QObject::connect(
		process,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, process](int exitCode, QProcess::ExitStatus) {
			if (exitCode == 0) {
				const QString output = QString::fromUtf8(process->readAllStandardOutput());
				this->parseIfconfigOutput(output);
			}

			this->mPendingProcesses.removeOne(process);
			process->deleteLater();
		}
	);

	process->start("ifconfig", QStringList() << this->mInterfaceName);
}

void FreeBSDWifiDevice::parseIfconfigOutput(const QString& output) {
	const QRegularExpression macRegex(R"(ether\s+([0-9a-f:]{17}))");
	auto match = macRegex.match(output);

	if (match.hasMatch()) {
		this->updateAddress(match.captured(1));
	}

	// Extract current SSID (handle both quoted and unquoted formats)
	const QRegularExpression ssidRegex(R"regex(ssid\s+(?:"([^"]+)"|(\S+)))regex");
	match = ssidRegex.match(output);

	if (match.hasMatch()) {
		// captured(1) is quoted SSID, captured(2) is unquoted SSID
		this->mCurrentSsid = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
	} else {
		this->mCurrentSsid.clear();
	}

	// Extract current BSSID from ifconfig output
	const QRegularExpression bssidRegex(R"(bssid\s+([0-9a-f:]{17}))", QRegularExpression::CaseInsensitiveOption);
	match = bssidRegex.match(output);

	if (match.hasMatch()) {
		this->mCurrentBssid = match.captured(1).toLower();
	} else {
		this->mCurrentBssid.clear();
		qCDebug(logNetworkFreeBSD) << "No BSSID found in the ifconfig output";
	}

	const bool isUp = output.contains("status: active") || output.contains("status: associated");
	this->updateConnectionState(
	    isUp ? DeviceConnectionState::Connected : DeviceConnectionState::Disconnected
	);

	if (output.contains("mediaopt adhoc")) {
		this->updateMode(WifiDeviceMode::AdHoc);
	} else if (output.contains("mediaopt hostap")) {
		this->updateMode(WifiDeviceMode::AccessPoint);
	} else if (output.contains("mediaopt mesh")) {
		this->updateMode(WifiDeviceMode::Mesh);
	} else {
		this->updateMode(WifiDeviceMode::Station);
	}

	qCDebug(logNetworkFreeBSD) << "Current SSID:" << this->mCurrentSsid;
	qCDebug(logNetworkFreeBSD) << "Current BSSID:" << this->mCurrentBssid;

	// Update connection state based on current BSSID
	for (auto it = this->mNetworkMap.begin(); it != this->mNetworkMap.end(); ++it) {
		const QString& networkBssid = it.key();  // BSSID is the map key
		auto* network = it.value();

		const bool isMatch = isUp && !this->mCurrentBssid.isEmpty() && (networkBssid == this->mCurrentBssid);

		if (isMatch) {
			network->updateConnectionState(NetworkState::Connected);
		} else {
			network->updateConnectionState(NetworkState::Disconnected);
		}
	}
}

void FreeBSDWifiDevice::onDisconnectRequested() {
	qCInfo(logNetworkFreeBSD) << "Disconnecting" << this->mInterfaceName;

	this->updateConnectionState(DeviceConnectionState::Disconnecting);

	auto* process = new QProcess(this);
	this->mPendingProcesses.append(process);

	QObject::connect(
		process,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, process](int, QProcess::ExitStatus) {
			this->updateStateFromIfconfig();
			this->mPendingProcesses.removeOne(process);

			process->deleteLater();
		}
	);

	process->start("ifconfig", QStringList() << this->mInterfaceName << "down");
}

void FreeBSDWifiDevice::onSetAutoconnectRequested(bool autoconnect) {
	this->mAutoconnect = autoconnect;
	qCInfo(logNetworkFreeBSD) << "Autoconnect for" << this->mInterfaceName << "set to" << autoconnect;
	emit this->autoconnectChanged();
}

// FreeBSDWifiNetwork implementation
FreeBSDWifiNetwork::FreeBSDWifiNetwork(QString ssid, QString bssid, QString interfaceName, QObject* parent)
    : WifiNetwork(std::move(ssid), parent)
    , mBssid(std::move(bssid))
    , mInterfaceName(std::move(interfaceName)) {
	QObject::connect(
	    this,
	    &WifiNetwork::requestConnect,
	    this,
	    &FreeBSDWifiNetwork::onConnectRequested
	);

	QObject::connect(
	    this,
	    &WifiNetwork::requestDisconnect,
	    this,
	    &FreeBSDWifiNetwork::onDisconnectRequested
	);

	QObject::connect(this, &WifiNetwork::requestForget, this, &FreeBSDWifiNetwork::onForgetRequested);
}

FreeBSDWifiNetwork::~FreeBSDWifiNetwork() {
	for (auto* process : this->mPendingProcesses) {
		if (process->state() != QProcess::NotRunning) {
			process->kill();
			process->waitForFinished(1000);
		}

		delete process;
	}

	this->mPendingProcesses.clear();
}

void FreeBSDWifiNetwork::updateSignalStrength(qreal strength) {
	this->bindableSignalStrength().setValue(strength);
}

void FreeBSDWifiNetwork::updateSecurity(WifiSecurityType::Enum security) {
	this->bindableSecurity().setValue(security);
}

void FreeBSDWifiNetwork::updateKnown(bool known) { this->bindableKnown().setValue(known); }

void FreeBSDWifiNetwork::updateConnectionState(NetworkState::Enum state) {
	this->bindableState().setValue(state);
	this->bindableConnected().setValue(state == NetworkState::Connected);
}

void FreeBSDWifiNetwork::onConnectRequested() {
	qCInfo(logNetworkFreeBSD) << "Connecting to" << this->name() << "BSSID:" << this->mBssid << "on" << this->mInterfaceName;

	this->updateConnectionState(NetworkState::Connecting);

	auto* process = new QProcess(this);
	this->mPendingProcesses.append(process);

	QObject::connect(
		process,
		QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this,
		[this, process](int exitCode, QProcess::ExitStatus) {
			if (exitCode == 0) {
				this->updateConnectionState(NetworkState::Connected);
			} else {
				qCWarning(logNetworkFreeBSD) << "Failed to connect to" << this->name() << "BSSID:" << this->mBssid;
				this->updateConnectionState(NetworkState::Disconnected);
			}

			this->mPendingProcesses.removeOne(process);
			process->deleteLater();
		}
	);

	// Connect to specific BSSID to avoid ambiguity
	process->start(
		"ifconfig",
		QStringList() << this->mInterfaceName << "ssid" << this->name() << "bssid" << this->mBssid << "up"
	);
}

void FreeBSDWifiNetwork::onDisconnectRequested() {
	qCInfo(logNetworkFreeBSD) << "Disconnecting from" << this->name() << "BSSID:" << this->mBssid;

	this->updateConnectionState(NetworkState::Disconnecting);

	auto* process = new QProcess(this);
	QObject::connect(
	    process,
	    QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
	    this,
	    [this, process](int, QProcess::ExitStatus) {
		    this->updateConnectionState(NetworkState::Disconnected);
		    process->deleteLater();
	    }
	);

	process->start("ifconfig", QStringList() << this->mInterfaceName << "down");
}

void FreeBSDWifiNetwork::onForgetRequested() {
	qCInfo(logNetworkFreeBSD) << "Forgetting network" << this->name() << "BSSID:" << this->mBssid;
	this->updateKnown(false);
}

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::FreeBSDWiredDevice* device) {
	auto saver = QDebugStateSaver(debug);

	if (device) {
		debug.nospace() << "WiredDevice(" << static_cast<const void*>(device)
		                << ", name=" << device->name() << ")";
	} else {
		debug << "WiredDevice(nullptr)";
	}

	return debug;
}

QDebug operator<<(QDebug debug, const qs::network::FreeBSDWifiDevice* device) {
	auto saver = QDebugStateSaver(debug);

	if (device) {
		debug.nospace() << "WifiDevice(" << static_cast<const void*>(device)
		                << ", name=" << device->name() << ")";
	} else {
		debug << "WifiDevice(nullptr)";
	}

	return debug;
}

QDebug operator<<(QDebug debug, const qs::network::FreeBSDWifiNetwork* network) {
	auto saver = QDebugStateSaver(debug);

	if (network) {
		debug.nospace() << "WifiNetwork(" << static_cast<const void*>(network)
		                << ", name=" << network->name() << ")";
	} else {
		debug << "WifiNetwork(nullptr)";
	}

	return debug;
}
