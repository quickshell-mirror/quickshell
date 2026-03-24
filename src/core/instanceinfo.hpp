#pragma once

#include <qdatetime.h>
#include <qlogging.h>
#include <qstring.h>
#include <sys/types.h>

struct InstanceInfo {
	Q_GADGET

	Q_PROPERTY(QString instanceId MEMBER instanceId CONSTANT)
	Q_PROPERTY(QString shellId MEMBER shellId CONSTANT)
	Q_PROPERTY(QString appId MEMBER appId CONSTANT)
	Q_PROPERTY(QDateTime launchTime MEMBER launchTime CONSTANT)

public:
	QString instanceId;
	QString configPath;
	QString shellId;
	QString appId;
	QDateTime launchTime;
	pid_t pid = -1;
	QString display;

	static InstanceInfo CURRENT; // NOLINT
};

struct RelaunchInfo {
	InstanceInfo instance;
	bool noColor = false;
	bool timestamp = false;
	bool sparseLogsOnly = false;
	QtMsgType defaultLogLevel = QtWarningMsg;
	QString logRules;
};

QDataStream& operator<<(QDataStream& stream, const InstanceInfo& info);
QDataStream& operator>>(QDataStream& stream, InstanceInfo& info);

QDataStream& operator<<(QDataStream& stream, const RelaunchInfo& info);
QDataStream& operator>>(QDataStream& stream, RelaunchInfo& info);

namespace qs::crash {

struct CrashInfo {
	int logFd = -1;
	int traceFd = -1;
	int infoFd = -1;

	static CrashInfo INSTANCE; // NOLINT
};

} // namespace qs::crash
