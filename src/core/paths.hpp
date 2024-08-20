#pragma once
#include <qdatetime.h>
#include <qdir.h>

class QsPaths {
public:
	static QsPaths* instance();
	static void init(QString shellId);
	static QDir crashDir(const QString& shellId, const QDateTime& launchTime);

	QDir* cacheDir();
	QDir* runDir();
	QDir* instanceRunDir();

private:
	enum class DirState {
		Unknown = 0,
		Ready = 1,
		Failed = 2,
	};

	QString shellId;
	QDir mCacheDir;
	QDir mRunDir;
	QDir mInstanceRunDir;
	DirState cacheState = DirState::Unknown;
	DirState runState = DirState::Unknown;
	DirState instanceRunState = DirState::Unknown;
};
