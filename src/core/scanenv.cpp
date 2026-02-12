#include "scanenv.hpp"

#include <qcontainerfwd.h>

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

} // namespace qs::scan::env
