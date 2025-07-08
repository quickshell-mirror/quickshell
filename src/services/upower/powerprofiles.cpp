#include "powerprofiles.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qdbusinterface.h>
#include <qdbusmetatype.h>
#include <qdebug.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringliteral.h>

#include "../../core/logcat.hpp"
#include "../../dbus/bus.hpp"
#include "../../dbus/properties.hpp"

namespace qs::service::upower {

namespace {
QS_LOGGING_CATEGORY(logPowerProfiles, "quickshell.service.powerprofiles", QtWarningMsg);
}

QString PowerProfile::toString(PowerProfile::Enum profile) {
	switch (profile) {
	case PowerProfile::PowerSaver: return QStringLiteral("PowerSaver");
	case PowerProfile::Balanced: return QStringLiteral("Balanced");
	case PowerProfile::Performance: return QStringLiteral("Performance");
	default: return QStringLiteral("Invalid");
	}
}

QString PerformanceDegradationReason::toString(PerformanceDegradationReason::Enum reason) {
	switch (reason) {
	case PerformanceDegradationReason::LapDetected: return QStringLiteral("LapDetected");
	case PerformanceDegradationReason::HighTemperature: return QStringLiteral("HighTemperature");
	default: return QStringLiteral("Invalid");
	}
}

bool PowerProfileHold::operator==(const PowerProfileHold& other) const {
	return other.profile == this->profile && other.applicationId == this->applicationId
	    && other.reason == this->reason;
}

QDebug& operator<<(QDebug& debug, const PowerProfileHold& hold) {
	auto saver = QDebugStateSaver(debug);

	debug.nospace();
	debug << "PowerProfileHold(profile=" << hold.profile << ", applicationId=" << hold.applicationId
	      << ", reason=" << hold.reason << ')';

	return debug;
}

PowerProfiles::PowerProfiles() {
	qDBusRegisterMetaType<QList<QVariantMap>>();

	this->bHasPerformanceProfile.setBinding([this]() {
		return this->bProfiles.value().contains(PowerProfile::Performance);
	});

	qCDebug(logPowerProfiles) << "Starting PowerProfiles Service.";

	auto bus = QDBusConnection::systemBus();

	if (!bus.isConnected()) {
		qCWarning(logPowerProfiles
		) << "Could not connect to DBus. PowerProfiles services will not work.";
	}

	this->service = new QDBusInterface(
	    "org.freedesktop.UPower.PowerProfiles",
	    "/org/freedesktop/UPower/PowerProfiles",
	    "org.freedesktop.UPower.PowerProfiles",
	    bus,
	    this
	);

	if (!this->service->isValid()) {
		qCDebug(logPowerProfiles
		) << "PowerProfilesDaemon is not currently running, attempting to start it.";

		dbus::tryLaunchService(this, bus, "org.freedesktop.UPower.PowerProfiles", [this](bool success) {
			if (success) {
				qCDebug(logPowerProfiles) << "Successfully launched PowerProfiles service.";
				this->init();
			} else {
				qCWarning(logPowerProfiles)
				    << "Could not start PowerProfilesDaemon. The PowerProfiles service will not work.";
			}
		});
	} else {
		this->init();
	}
}

void PowerProfiles::init() {
	this->properties.setInterface(this->service);
	this->properties.updateAllViaGetAll();
}

void PowerProfiles::setProfile(PowerProfile::Enum profile) {
	if (!this->properties.isConnected()) {
		qCCritical(logPowerProfiles
		) << "Cannot set power profile: power-profiles-daemon not accessible or not running";
		return;
	}

	if (profile == PowerProfile::Performance && !this->bHasPerformanceProfile) {
		qCCritical(logPowerProfiles
		) << "Cannot request performance profile as it is not present for this device.";
		return;
	} else if (profile < PowerProfile::PowerSaver || profile > PowerProfile::Performance) {
		qCCritical(logPowerProfiles) << "Tried to request invalid power profile" << profile;
		return;
	}

	this->bProfile = profile;
	this->pProfile.write();
}

PowerProfiles* PowerProfiles::instance() {
	static auto* instance = new PowerProfiles(); // NOLINT
	return instance;
}

PowerProfilesQml::PowerProfilesQml(QObject* parent): QObject(parent) {
	auto* instance = PowerProfiles::instance();

	this->bProfile.setBinding([instance]() { return instance->bProfile.value(); });

	this->bHasPerformanceProfile.setBinding([instance]() {
		return instance->bHasPerformanceProfile.value();
	});

	this->bDegradationReason.setBinding([instance]() { return instance->bDegradationReason.value(); }
	);

	this->bHolds.setBinding([instance]() { return instance->bHolds.value(); });
}

} // namespace qs::service::upower

namespace qs::dbus {

using namespace qs::service::upower;

DBusResult<PowerProfile::Enum> DBusDataTransform<PowerProfile::Enum>::fromWire(const Wire& wire) {
	if (wire == QStringLiteral("power-saver")) {
		return PowerProfile::PowerSaver;
	} else if (wire == QStringLiteral("balanced")) {
		return PowerProfile::Balanced;
	} else if (wire == QStringLiteral("performance")) {
		return PowerProfile::Performance;
	} else {
		return QDBusError(QDBusError::InvalidArgs, QString("Invalid PowerProfile: %1").arg(wire));
	}
}

QString DBusDataTransform<PowerProfile::Enum>::toWire(Data data) {
	switch (data) {
	case PowerProfile::PowerSaver: return QStringLiteral("power-saver");
	case PowerProfile::Balanced: return QStringLiteral("balanced");
	case PowerProfile::Performance: return QStringLiteral("performance");
	}
}

DBusResult<QList<PowerProfile::Enum>>
DBusDataTransform<QList<PowerProfile::Enum>>::fromWire(const Wire& wire) {
	QList<PowerProfile::Enum> profiles;

	for (const auto& entry: wire) {
		auto profile =
		    DBusDataTransform<PowerProfile::Enum>::fromWire(entry.value("Profile").value<QString>());

		if (!profile.isValid()) return profile.error;
		profiles.append(profile.value);
	}

	return profiles;
}

DBusResult<PerformanceDegradationReason::Enum>
DBusDataTransform<PerformanceDegradationReason::Enum>::fromWire(const Wire& wire) {
	if (wire.isEmpty()) {
		return PerformanceDegradationReason::None;
	} else if (wire == QStringLiteral("lap-detected")) {
		return PerformanceDegradationReason::LapDetected;
	} else if (wire == QStringLiteral("high-operating-temperature")) {
		return PerformanceDegradationReason::HighTemperature;
	} else {
		return QDBusError(
		    QDBusError::InvalidArgs,
		    QString("Invalid PerformanceDegradationReason: %1").arg(wire)
		);
	}
}

DBusResult<QList<PowerProfileHold>>
DBusDataTransform<QList<PowerProfileHold>>::fromWire(const Wire& wire) {
	QList<PowerProfileHold> holds;

	for (const auto& entry: wire) {
		auto profile =
		    DBusDataTransform<PowerProfile::Enum>::fromWire(entry.value("Profile").value<QString>());

		if (!profile.isValid()) return profile.error;

		auto applicationId = entry.value("ApplicationId").value<QString>();
		auto reason = entry.value("Reason").value<QString>();

		holds.append(PowerProfileHold(profile.value, applicationId, reason));
	}

	return holds;
}

} // namespace qs::dbus
