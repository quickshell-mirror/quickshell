#pragma once

#include <qdatetime.h>
#include <qprocess.h>

namespace qs {

struct Common {
	static const QDateTime LAUNCH_TIME;
	static QProcessEnvironment INITIAL_ENVIRONMENT; // NOLINT
};

} // namespace qs
