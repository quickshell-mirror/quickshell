#include "paths.hpp"
#include <utility>

#include <qdatetime.h>
#include <qdir.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstandardpaths.h>
#include <qtenvironmentvariables.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(logPaths, "quickshell.paths", QtWarningMsg);

QsPaths* QsPaths::instance() {
	static auto* instance = new QsPaths(); // NOLINT
	return instance;
}

void QsPaths::init(QString shellId) { QsPaths::instance()->shellId = std::move(shellId); }

QDir* QsPaths::cacheDir() {
	if (this->cacheState == DirState::Unknown) {
		auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
		dir = QDir(dir.filePath("quickshell"));
		dir = QDir(dir.filePath(this->shellId));
		this->mCacheDir = dir;

		qCDebug(logPaths) << "Initialized cache path:" << dir.path();

		if (!dir.mkpath(".")) {
			qCCritical(logPaths) << "Cannot create cache directory at" << dir.path();

			this->cacheState = DirState::Failed;
		}
	}

	if (this->cacheState == DirState::Failed) return nullptr;
	else return &this->mCacheDir;
}

QDir* QsPaths::runDir() {
	if (this->runState == DirState::Unknown) {
		auto runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
		if (runtimeDir.isEmpty()) {
			runtimeDir = QString("/run/user/$1").arg(getuid());
			qCInfo(logPaths) << "XDG_RUNTIME_DIR was not set, defaulting to" << runtimeDir;
		}

		auto dir = QDir(runtimeDir);
		dir = QDir(dir.filePath("quickshell"));
		dir = QDir(dir.filePath(this->shellId));
		this->mRunDir = dir;

		qCDebug(logPaths) << "Initialized runtime path:" << dir.path();

		if (!dir.mkpath(".")) {
			qCCritical(logPaths) << "Cannot create runtime directory at" << dir.path();

			this->runState = DirState::Failed;
		}
	}

	if (this->runState == DirState::Failed) return nullptr;
	else return &this->mRunDir;
}

QDir* QsPaths::instanceRunDir() {
	if (this->instanceRunState == DirState::Unknown) {
		auto* runtimeDir = this->runDir();

		if (!runtimeDir) {
			qCCritical(logPaths) << "Cannot create instance runtime directory as main runtim directory "
			                        "could not be created.";
			this->instanceRunState = DirState::Failed;
		} else {
			this->mInstanceRunDir =
			    runtimeDir->filePath(QString("run-%1").arg(QDateTime::currentMSecsSinceEpoch()));

			qCDebug(logPaths) << "Initialized instance runtime path:" << this->mInstanceRunDir.path();

			if (!this->mInstanceRunDir.mkpath(".")) {
				qCCritical(logPaths) << "Cannot create instance runtime directory at"
				                     << this->mInstanceRunDir.path();
				this->instanceRunState = DirState::Failed;
			}
		}
	}

	if (this->runState == DirState::Failed) return nullptr;
	else return &this->mInstanceRunDir;
}
