#pragma once

#include <qtclasshelpermacros.h>

#include "../core/crashinfo.hpp"
namespace qs::crash {

struct CrashHandlerPrivate;

class CrashHandler {
public:
	explicit CrashHandler();
	~CrashHandler();
	Q_DISABLE_COPY_MOVE(CrashHandler);

	void init();
	void setInstanceInfo(const InstanceInfo& info);

private:
	CrashHandlerPrivate* d;
};

} // namespace qs::crash
