#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "enums.hpp"

namespace qs::network {

///! A wireless access point belonging to a @@WifiNetwork.
class AccessPoint: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Access points can only be acquired through WifiNetwork");

	// clang-format off
	/// The basic service set ID (MAC address) of the access point, in XX:XX:XX:XX:XX:XX format.
	Q_PROPERTY(QString bssid READ default NOTIFY bssidChanged BINDABLE bindableBssid);
	/// The signal strength of the access point from 0.0 to 1.0.
	Q_PROPERTY(qreal signalStrength READ default NOTIFY signalStrengthChanged BINDABLE bindableSignalStrength);
	/// The best security type supported by both the access point and the device.
	Q_PROPERTY(qs::network::WifiSecurityType::Enum security READ default NOTIFY securityChanged BINDABLE bindableSecurity);
	/// The frequency of the access point in MHz.
	Q_PROPERTY(quint32 frequency READ default NOTIFY frequencyChanged BINDABLE bindableFrequency);
	/// True if the network is connected using this access point.
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	// clang-format on

public:
	explicit AccessPoint(QObject* parent = nullptr): QObject(parent) {}

	QBindable<QString> bindableBssid() { return &this->bBssid; }
	QBindable<qreal> bindableSignalStrength() { return &this->bSignalStrength; }
	QBindable<quint32> bindableFrequency() { return &this->bFrequency; }
	QBindable<bool> bindableActive() { return &this->bActive; }
	QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; }

signals:
	void bssidChanged();
	void signalStrengthChanged();
	void frequencyChanged();
	void activeChanged();
	void securityChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(AccessPoint, QString, bBssid, &AccessPoint::bssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(AccessPoint, qreal, bSignalStrength, &AccessPoint::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(AccessPoint, quint32, bFrequency, &AccessPoint::frequencyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(AccessPoint, bool, bActive, &AccessPoint::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(AccessPoint, WifiSecurityType::Enum, bSecurity, WifiSecurityType::Unknown, &AccessPoint::securityChanged);
	// clang-format on
};

} // namespace qs::network
