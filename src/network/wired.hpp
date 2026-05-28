#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "device.hpp"
#include "network.hpp"

namespace qs::network {

///! Wired variant of a @@NetworkDevice.
class WiredDevice: public NetworkDevice {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	/// The wired network for this device or `null`.
	///
	/// > [!NOTE] This network is only available when @@hasLink is `true`.
	Q_PROPERTY(Network* network READ network NOTIFY networkChanged BINDABLE bindableNetwork);
	/// The maximum speed of the physical device link, in megabits per second.
	Q_PROPERTY(quint32 linkSpeed READ default NOTIFY linkSpeedChanged BINDABLE bindableLinkSpeed);
	/// True if the wired device has a physical link (cable plugged in).
	Q_PROPERTY(bool hasLink READ default NOTIFY hasLinkChanged BINDABLE bindableHasLink);

public:
	explicit WiredDevice(QObject* parent = nullptr);

	void networkAdded(Network* net) override;
	void networkRemoved(Network* net) override;

	[[nodiscard]] Network* network() const { return this->bNetwork; }
	QBindable<Network*> bindableNetwork() { return &this->bNetwork; }
	QBindable<quint32> bindableLinkSpeed() { return &this->bLinkSpeed; }
	QBindable<bool> bindableHasLink() { return &this->bHasLink; }

signals:
	void networkChanged();
	void linkSpeedChanged();
	void hasLinkChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WiredDevice, Network*, bNetwork, &WiredDevice::networkChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WiredDevice, quint32, bLinkSpeed, &WiredDevice::linkSpeedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WiredDevice, bool, bHasLink, &WiredDevice::hasLinkChanged);
	// clang-format on
};

} // namespace qs::network
