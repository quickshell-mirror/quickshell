#include "scanenv.hpp"

#include <qcontainerfwd.h>
#include <qtenvironmentvariables.h>
#include <qtversion.h>
#include <qversionnumber.h>

#include "build.hpp"

namespace qs::scan::env {

bool PreprocEnv::hasVersion(int major, int minor, const QStringList& features) {
	if (QS_VERSION_MAJOR > major) return true;
	if (QS_VERSION_MAJOR == major && QS_VERSION_MINOR > minor) return true;

	auto availFeatures = QString(QS_UNRELEASED_FEATURES).split(';');

	for (const auto& feature: features) {
		if (!availFeatures.contains(feature)) return false;
	}

	return QS_VERSION_MAJOR == major && QS_VERSION_MINOR == minor;
}

bool PreprocEnv::hasQtVersion(int major, int minor) {
	auto qtVersion = QVersionNumber::fromString(qVersion());
	auto requiredVersion = QVersionNumber(major, minor);
	return qtVersion >= requiredVersion;
}

QString PreprocEnv::env(const QString& variable) {
	return qEnvironmentVariable(variable.toStdString().c_str());
}

bool PreprocEnv::isEnvSet(const QString& variable) {
	return qEnvironmentVariableIsSet(variable.toStdString().c_str());
}

} // namespace qs::scan::env
