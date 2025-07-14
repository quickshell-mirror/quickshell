#pragma once

#include <qdatetime.h>
#include <qprocess.h>

namespace qs {

struct Common {
	static const QDateTime LAUNCH_TIME;
	static inline QProcessEnvironment INITIAL_ENVIRONMENT = {}; // NOLINT
};

} // namespace qs
