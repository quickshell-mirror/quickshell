#pragma once

#include <qtclasshelpermacros.h>

#include "../core/instanceinfo.hpp"
namespace qs::crash {

struct CrashHandlerPrivate;

class CrashHandler {
public:
	explicit CrashHandler();
	~CrashHandler();
	Q_DISABLE_COPY_MOVE(CrashHandler);

	void init();
	void setRelaunchInfo(const RelaunchInfo& info);

private:
	CrashHandlerPrivate* d;
};

} // namespace qs::crash
