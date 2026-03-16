#pragma once

#include <qcontainerfwd.h>

namespace qs::debuginfo {

QString qsVersion();
QString qtVersion();
QString gpuInfo();
QString systemInfo();
QString envInfo();
QString combinedInfo();

} // namespace qs::debuginfo
