#include "common.hpp"

#include <qdatetime.h>
#include <qprocess.h>

namespace qs {

const QDateTime Common::LAUNCH_TIME = QDateTime::currentDateTime();
QProcessEnvironment Common::INITIAL_ENVIRONMENT = {}; // NOLINT

} // namespace qs
