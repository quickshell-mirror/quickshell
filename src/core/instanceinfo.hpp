#pragma once

#include <qdatetime.h>
#include <qstring.h>

struct InstanceInfo {
	QString instanceId;
	QString configPath;
	QString shellId;
	QString initialWorkdir;
	QDateTime launchTime;

	static InstanceInfo CURRENT; // NOLINT
};

struct RelaunchInfo {
	InstanceInfo instance;
	bool noColor = false;
	bool sparseLogsOnly = false;
};

QDataStream& operator<<(QDataStream& stream, const InstanceInfo& info);
QDataStream& operator>>(QDataStream& stream, InstanceInfo& info);

QDataStream& operator<<(QDataStream& stream, const RelaunchInfo& info);
QDataStream& operator>>(QDataStream& stream, RelaunchInfo& info);

namespace qs::crash {

struct CrashInfo {
	int logFd = -1;

	static CrashInfo INSTANCE; // NOLINT
};

} // namespace qs::crash
