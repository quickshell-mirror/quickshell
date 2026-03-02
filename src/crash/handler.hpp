#pragma once

#include <qtclasshelpermacros.h>

#include "../core/instanceinfo.hpp"
namespace qs::crash {

class CrashHandler {
public:
	static void init();
	static void setRelaunchInfo(const RelaunchInfo& info);
};

} // namespace qs::crash
