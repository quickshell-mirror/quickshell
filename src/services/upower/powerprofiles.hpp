#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qdbusinterface.h>
#include <qdebug.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"

namespace qs::service::upower {

///! Power profile exposed by the PowerProfiles service.
/// See @@PowerProfiles.
class PowerProfile: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// This profile will limit system performance in order to save power.
		PowerSaver = 0,
		/// This profile is the default, and will attempt to strike a balance
		/// between performance and power consumption.
		Balanced = 1,
		/// This profile will maximize performance at the cost of power consumption.
		Performance = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(qs::service::upower::PowerProfile::Enum profile);
};

///! Reason for performance degradation exposed by the PowerProfiles service.
/// See @@PowerProfiles.degradationReason for more information.
class PerformanceDegradationReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// Performance has not been degraded in a way power-profiles-daemon can detect.
		None = 0,
		/// Performance has been reduced due to the computer's lap detection function,
		/// which attempts to keep the computer from getting too hot while on your lap.
		LapDetected = 1,
		/// Performance has been reduced due to high system temperatures.
		HighTemperature = 2,
	};
	Q_ENUM(Enum);

	// clang-format off
	Q_INVOKABLE static QString toString(qs::service::upower::PerformanceDegradationReason::Enum reason);
	// clang-format on
};

class PowerProfileHold;

} // namespace qs::service::upower

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::upower::PowerProfile::Enum> {
	using Wire = QString;
	using Data = qs::service::upower::PowerProfile::Enum;
	static DBusResult<Data> fromWire(const Wire& wire);
	static Wire toWire(Data data);
};

template <>
struct DBusDataTransform<QList<qs::service::upower::PowerProfile::Enum>> {
	using Wire = QList<QVariantMap>;
	using Data = QList<qs::service::upower::PowerProfile::Enum>;
	static DBusResult<Data> fromWire(const Wire& wire);
};

template <>
struct DBusDataTransform<qs::service::upower::PerformanceDegradationReason::Enum> {
	using Wire = QString;
	using Data = qs::service::upower::PerformanceDegradationReason::Enum;
	static DBusResult<Data> fromWire(const Wire& wire);
};

template <>
struct DBusDataTransform<QList<qs::service::upower::PowerProfileHold>> {
	using Wire = QList<QVariantMap>;
	using Data = QList<qs::service::upower::PowerProfileHold>;
	static DBusResult<Data> fromWire(const Wire& wire);
};

} // namespace qs::dbus

namespace qs::service::upower {

// docgen can't hit gadgets yet
class PowerProfileHold {
	Q_GADGET;
	QML_VALUE_TYPE(powerProfileHold);
	Q_PROPERTY(qs::service::upower::PowerProfile::Enum profile MEMBER profile CONSTANT);
	Q_PROPERTY(QString applicationId MEMBER applicationId CONSTANT);
	Q_PROPERTY(QString reason MEMBER reason CONSTANT);

public:
	explicit PowerProfileHold() = default;
	explicit PowerProfileHold(PowerProfile::Enum profile, QString applicationId, QString reason)
	    : profile(profile)
	    , applicationId(std::move(applicationId))
	    , reason(std::move(reason)) {}

	PowerProfile::Enum profile = PowerProfile::Balanced;
	QString applicationId;
	QString reason;

	[[nodiscard]] bool operator==(const PowerProfileHold& other) const;
};

QDebug& operator<<(QDebug& debug, const PowerProfileHold& hold);

class PowerProfiles: public QObject {
	Q_OBJECT;

public:
	void setProfile(PowerProfile::Enum profile);

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerProfiles, PowerProfile::Enum, bProfile, PowerProfile::Balanced);
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfiles, bool, bHasPerformanceProfile);
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfiles, PerformanceDegradationReason::Enum, bDegradationReason);
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfiles, QList<PowerProfileHold>, bHolds);
	// clang-format on

	static PowerProfiles* instance();

private:
	explicit PowerProfiles();
	void init();

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfiles, QList<PowerProfile::Enum>, bProfiles);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(PowerProfiles, properties);
	QS_DBUS_PROPERTY_BINDING(PowerProfiles, pProfile, bProfile, properties, "ActiveProfile");
	QS_DBUS_PROPERTY_BINDING(PowerProfiles, pProfiles, bProfiles, properties, "Profiles");
	QS_DBUS_PROPERTY_BINDING(PowerProfiles, pPerformanceDegraded, bDegradationReason, properties, "PerformanceDegraded");
	QS_DBUS_PROPERTY_BINDING(PowerProfiles, pHolds, bHolds, properties, "ActiveProfileHolds");
	// clang-format on

	QDBusInterface* service = nullptr;
};

///! Provides access to the Power Profiles service.
/// An interface to the UPower [power profiles daemon], which can be
/// used to view and manage power profiles.
///
/// > [!NOTE] The power profiles daemon must be installed to use this service.
/// > Installing UPower does not necessarily install the power profiles daemon.
///
/// [power profiles daemon]: https://gitlab.freedesktop.org/upower/power-profiles-daemon
class PowerProfilesQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(PowerProfiles);
	QML_SINGLETON;
	// clang-format off
	/// The current power profile.
	///
	/// This property may be set to change the system's power profile, however
	/// it cannot be set to `Performance` unless @@hasPerformanceProfile is true.
	Q_PROPERTY(qs::service::upower::PowerProfile::Enum profile READ default WRITE setProfile NOTIFY profileChanged BINDABLE bindableProfile);
	/// If the system has a performance profile.
	///
	/// If this property is false, your system does not have a performance
	/// profile known to power-profiles-daemon.
	Q_PROPERTY(bool hasPerformanceProfile READ default NOTIFY hasPerformanceProfileChanged BINDABLE bindableHasPerformanceProfile);
	/// If power-profiles-daemon detects degraded system performance, the reason
	/// for the degradation will be present here.
	Q_PROPERTY(qs::service::upower::PerformanceDegradationReason::Enum degradationReason READ default NOTIFY degradationReasonChanged BINDABLE bindableDegradationReason);
	/// Power profile holds created by other applications.
	///
	/// This property returns a `powerProfileHold` object, which has the following properties.
	/// - `profile` - The @@PowerProfile held by the application.
	/// - `applicationId` - A string identifying the application
	/// - `reason` - The reason the application has given for holding the profile.
	///
	/// Applications may "hold" a power profile in place for their lifetime, such
	/// as a game holding Performance mode or a system daemon holding Power Saver mode
	/// when reaching a battery threshold. If the user selects a different profile explicitly
	/// (e.g. by setting @@profile$) all holds will be removed.
	///
	/// Multiple applications may hold a power profile, however if multiple applications request
	/// profiles than `PowerSaver` will win over `Performance`. Only `Performance` and `PowerSaver`
	/// profiles may be held.
	Q_PROPERTY(QList<qs::service::upower::PowerProfileHold> holds READ default NOTIFY holdsChanged BINDABLE bindableHolds);
	// clang-format on

signals:
	void profileChanged();
	void hasPerformanceProfileChanged();
	void degradationReasonChanged();
	void holdsChanged();

public:
	explicit PowerProfilesQml(QObject* parent = nullptr);

	[[nodiscard]] QBindable<PowerProfile::Enum> bindableProfile() const { return &this->bProfile; }

	static void setProfile(PowerProfile::Enum profile) {
		PowerProfiles::instance()->setProfile(profile);
	}

	[[nodiscard]] QBindable<bool> bindableHasPerformanceProfile() const {
		return &this->bHasPerformanceProfile;
	}

	[[nodiscard]] QBindable<PerformanceDegradationReason::Enum> bindableDegradationReason() const {
		return &this->bDegradationReason;
	}

	[[nodiscard]] QBindable<QList<PowerProfileHold>> bindableHolds() const { return &this->bHolds; }

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesQml, PowerProfile::Enum, bProfile, &PowerProfilesQml::profileChanged);
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesQml, bool, bHasPerformanceProfile, &PowerProfilesQml::hasPerformanceProfileChanged);
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesQml, PerformanceDegradationReason::Enum, bDegradationReason, &PowerProfilesQml::degradationReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesQml, QList<PowerProfileHold>, bHolds, &PowerProfilesQml::holdsChanged);
	// clang-format on
};

} // namespace qs::service::upower
