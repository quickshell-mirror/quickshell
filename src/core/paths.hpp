#pragma once
#include <qdatetime.h>
#include <qdir.h>

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
	static void init(QString shellId, QString pathId);
	static QDir crashDir(const QString& id);
	static QString basePath(const QString& id);
	static QString ipcPath(const QString& id);
	static bool checkLock(const QString& path, InstanceLockInfo* info = nullptr);
	static QVector<InstanceLockInfo> collectInstances(const QString& path);

	QDir* cacheDir();
	QDir* baseRunDir();
	QDir* shellRunDir();
	QDir* instanceRunDir();
	void linkRunDir();
	void linkPathDir();
	void createLock();

private:
	enum class DirState {
		Unknown = 0,
		Ready = 1,
		Failed = 2,
	};

	QString shellId;
	QString pathId;
	QDir mCacheDir;
	QDir mBaseRunDir;
	QDir mShellRunDir;
	QDir mInstanceRunDir;
	DirState cacheState = DirState::Unknown;
	DirState baseRunState = DirState::Unknown;
	DirState shellRunState = DirState::Unknown;
	DirState instanceRunState = DirState::Unknown;
};
