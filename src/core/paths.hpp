#pragma once
#include <qdatetime.h>
#include <qdir.h>
#include <qpair.h>
#include <qtypes.h>

#include "instanceinfo.hpp"

struct InstanceLockInfo {
	pid_t pid = -1;
	InstanceInfo instance;
};

QDataStream& operator<<(QDataStream& stream, const InstanceLockInfo& info);
QDataStream& operator>>(QDataStream& stream, InstanceLockInfo& info);

class QsPaths {
public:
	static QsPaths* instance();
	static void init(QString shellId, QString pathId, QString dataOverride, QString stateOverride);
	static QDir crashDir(const QString& id);
	static QString basePath(const QString& id);
	static QString ipcPath(const QString& id);
	static bool
	checkLock(const QString& path, InstanceLockInfo* info = nullptr, bool allowDead = false);
	static QPair<QVector<InstanceLockInfo>, QVector<InstanceLockInfo>>
	collectInstances(const QString& path);

	QDir* baseRunDir();
	QDir* shellRunDir();
	QDir* shellVfsDir();
	QDir* instanceRunDir();
	void linkRunDir();
	void linkPathDir();
	void createLock();

	QDir shellDataDir();
	QDir shellStateDir();
	QDir shellCacheDir();

private:
	enum class DirState : quint8 {
		Unknown = 0,
		Ready = 1,
		Failed = 2,
	};

	QString shellId;
	QString pathId;
	QDir mBaseRunDir;
	QDir mShellRunDir;
	QDir mShellVfsDir;
	QDir mInstanceRunDir;
	DirState baseRunState = DirState::Unknown;
	DirState shellRunState = DirState::Unknown;
	DirState shellVfsState = DirState::Unknown;
	DirState instanceRunState = DirState::Unknown;

	QDir mShellDataDir;
	QDir mShellStateDir;
	QDir mShellCacheDir;
	DirState shellDataState = DirState::Unknown;
	DirState shellStateState = DirState::Unknown;
	DirState shellCacheState = DirState::Unknown;

	QString shellDataOverride;
	QString shellStateOverride;
};
