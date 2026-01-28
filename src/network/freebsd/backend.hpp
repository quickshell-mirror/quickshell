#pragma once

#include <cstdint>

#include <QHash>
#include <QLocalSocket>
#include <QObject>
#include <QProcess>
#include <QProperty>
#include <QSocketNotifier>
#include <QStringList>
#include <QThread>

#include "../device.hpp"
#include "../network.hpp"
#include "../wifi.hpp"

namespace qs::network {

class FreeBSDWiredDevice;
class FreeBSDWifiNetwork;
class FreeBSDWifiDevice;

///! FreeBSD network backend implementation.
class FreeBSDBackend: public NetworkBackend {
	Q_OBJECT;

public:
	explicit FreeBSDBackend(QObject* parent = nullptr);
	~FreeBSDBackend() override;
	Q_DISABLE_COPY_MOVE(FreeBSDBackend);

	[[nodiscard]] bool isAvailable() const override;

	QBindable<bool> bindableWifiEnabled() { return &this->bWifiEnabled; }
	[[nodiscard]] bool wifiEnabled() const { return this->bWifiEnabled; }
	void setWifiEnabled(bool enabled);

	QBindable<bool> bindableWifiHardwareEnabled() { return &this->bWifiHardwareEnabled; }
	[[nodiscard]] bool wifiHardwareEnabled() const { return this->bWifiHardwareEnabled; }

signals:
	void deviceAdded(NetworkDevice* device);
	void deviceRemoved(NetworkDevice* device);
	void wifiEnabledChanged();
	void wifiHardwareEnabledChanged();

private slots:
	void onRouteSocketActivated();
	void onDevdActivated();

private:
	void initializeDevdSocket();
	void initializeRouteSocket();
	void cleanupSockets();
	void handleRouteMessage(const char* buf, ssize_t len);
	void handleDevdEvent(const QString& event);
	void processInterface(const QString& interfaceName, bool isNew);
	void removeInterface(const QString& interfaceName);
	void scanExistingDevices();

	QList<QProcess*> mPendingProcesses;
	QHash<QString, NetworkDevice*> mDevices;
	int mRouteSocket = -1;
	QSocketNotifier* mRouteNotifier = nullptr;
	int mDevdFd = -1;
	QSocketNotifier* mDevdNotifier = nullptr;
	QByteArray mDevdBuffer;

	Q_OBJECT_BINDABLE_PROPERTY(
	    FreeBSDBackend,
	    bool,
	    bWifiEnabled,
	    &FreeBSDBackend::wifiEnabledChanged
	);
	Q_OBJECT_BINDABLE_PROPERTY(
	    FreeBSDBackend,
	    bool,
	    bWifiHardwareEnabled,
	    &FreeBSDBackend::wifiHardwareEnabledChanged
	);
};

///! FreeBSD ethernet device implementation.
class FreeBSDWiredDevice: public NetworkDevice {
	Q_OBJECT;

public:
	explicit FreeBSDWiredDevice(const QString& interfaceName, QObject* parent = nullptr);
	~FreeBSDWiredDevice() override;
	Q_DISABLE_COPY_MOVE(FreeBSDWiredDevice);

	void handleInterfaceEvent();

private slots:
	void onDisconnectRequested();
	void onSetAutoconnectRequested(bool autoconnect);

private:
	friend class FreeBSDBackend;

	void updateName(const QString& name);
	void updateAddress(const QString& address);
	void updateConnectionState(DeviceConnectionState::Enum state);
	void updateStateFromIfconfig();
	void parseIfconfigOutput(const QString& output);

	QList<QProcess*> mPendingProcesses;
	QString mInterfaceName;
	bool mAutoconnect = true;
};

///! FreeBSD Wireless device implementation.
class FreeBSDWifiDevice: public WifiDevice {
	Q_OBJECT;

public:
	explicit FreeBSDWifiDevice(const QString& interfaceName, QObject* parent = nullptr);
	~FreeBSDWifiDevice() override;
	Q_DISABLE_COPY_MOVE(FreeBSDWifiDevice);

	void handleInterfaceEvent();
	void handleScanComplete();

private slots:
	void onDisconnectRequested();
	void onSetAutoconnectRequested(bool autoconnect);
	void onScanTimerTimeout();

private:
	friend class FreeBSDBackend;
	struct ScanResult {
		QString ssid;
		qreal strength;
		WifiSecurityType::Enum security;
	};

	void updateName(const QString& name);
	void updateAddress(const QString& address);
	void updateConnectionState(DeviceConnectionState::Enum state);
	void updateMode(WifiDeviceMode::Enum mode);

	void initializeSysctlNotifier();
	void triggerScan();
	void updateStateFromIfconfig();
	void parseIfconfigOutput(const QString& output);
	void loadKnownNetworks();
	void processScanLines(const QStringList& lines, int64_t startIndex);

	bool isNetworkKnown(const QString& ssid);

	QList<QProcess*> mPendingProcesses;
	QList<QThread*> mPendingThreads;
	QHash<QString, FreeBSDWifiNetwork*> mNetworkMap;
	QString mInterfaceName;
	QString mCurrentSsid;
	QString mCurrentBssid;
	bool mAutoconnect = true;
	bool mScanPending = false;
};

///! FreeBSD Wireless network implementation.
class FreeBSDWifiNetwork: public WifiNetwork {
	Q_OBJECT;

public:
	FreeBSDWifiNetwork(QString ssid, QString bssid, QString interfaceName, QObject* parent = nullptr);
	~FreeBSDWifiNetwork() override;
	Q_DISABLE_COPY_MOVE(FreeBSDWifiNetwork);

private slots:
	void onConnectRequested();
	void onDisconnectRequested();
	void onForgetRequested();

private:
	friend class FreeBSDWifiDevice;

	void updateSignalStrength(qreal strength);
	void updateSecurity(WifiSecurityType::Enum security);
	void updateKnown(bool known);
	void updateConnectionState(NetworkState::Enum state);

	QString mBssid;
	QList<QProcess*> mPendingProcesses;
	QString mInterfaceName;
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::FreeBSDWiredDevice* device);
QDebug operator<<(QDebug debug, const qs::network::FreeBSDWifiDevice* device);
QDebug operator<<(QDebug debug, const qs::network::FreeBSDWifiNetwork* network);
