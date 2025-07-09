#pragma once
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_wireless.h"

namespace qs::service::networkmanager {

class NMWirelessMode: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Adhoc = 1,
		Infra = 2,
		AP = 3,
		Mesh = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NMWirelessMode::Enum mode);
};

} // namespace qs::service::networkmanager

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::networkmanager::NMWirelessMode::Enum> {
	using Wire = quint32;
	using Data = qs::service::networkmanager::NMWirelessMode::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::service::networkmanager {

class NMWireless: public QObject {
	Q_OBJECT
	QML_ELEMENT;
	QML_UNCREATABLE("NMWireless can only be acquired from NMDevice");
	//clang-format off
	Q_PROPERTY(NMWirelessMode::Enum mode READ default NOTIFY modeChanged BINDABLE bindableMode);
	Q_PROPERTY(quint32 bitrate READ default NOTIFY bitrateChanged BINDABLE bindableBitrate);

public:
	explicit NMWireless(QObject* parent = nullptr);
	void init(const QString& path);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QBindable<NMWirelessMode::Enum> bindableMode() const { return &this->bMode; };	
	[[nodiscard]] QBindable<quint32> bindableBitrate() const { return &this->bBitrate; };	

signals:
	void modeChanged();
	void bitrateChanged();

private:
	Q_OBJECT_BINDABLE_PROPERTY(NMWireless, NMWirelessMode::Enum, bMode, &NMWireless::modeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWireless, quint32, bBitrate, &NMWireless::bitrateChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMWireless, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NMWireless, pMode, bMode, deviceProperties, "Mode");
	QS_DBUS_PROPERTY_BINDING(NMWireless, pBitrate, bBitrate, deviceProperties, "Bitrate");

	DBusNMWireless* wireless = nullptr;
};

} // namespace qs::service::networkmanager
