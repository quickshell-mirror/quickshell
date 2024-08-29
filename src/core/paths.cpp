#include "paths.hpp"
#include <cerrno>
#include <utility>

#include <qdatetime.h>
#include <qdir.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstandardpaths.h>
#include <qtenvironmentvariables.h>
#include <unistd.h>

#include "common.hpp"

Q_LOGGING_CATEGORY(logPaths, "quickshell.paths", QtWarningMsg);

QsPaths* QsPaths::instance() {
	static auto* instance = new QsPaths(); // NOLINT
	return instance;
}

void QsPaths::init(QString shellId) { QsPaths::instance()->shellId = std::move(shellId); }

QDir QsPaths::crashDir(const QString& shellId, const QDateTime& launchTime) {
	auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
	dir = QDir(dir.filePath("crashes"));
	dir = QDir(dir.filePath(shellId));
	dir = QDir(dir.filePath(QString("run-%1").arg(launchTime.toMSecsSinceEpoch())));

	return dir;
}

QDir* QsPaths::cacheDir() {
	if (this->cacheState == DirState::Unknown) {
		auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
		dir = QDir(dir.filePath(this->shellId));
		this->mCacheDir = dir;

		qCDebug(logPaths) << "Initialized cache path:" << dir.path();

		if (!dir.mkpath(".")) {
			qCCritical(logPaths) << "Could not create cache directory at" << dir.path();

			this->cacheState = DirState::Failed;
		} else {
			this->cacheState = DirState::Ready;
		}
	}

	if (this->cacheState == DirState::Failed) return nullptr;
	else return &this->mCacheDir;
}

QDir* QsPaths::baseRunDir() {
	if (this->baseRunState == DirState::Unknown) {
		auto runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
		if (runtimeDir.isEmpty()) {
			runtimeDir = QString("/run/user/$1").arg(getuid());
			qCInfo(logPaths) << "XDG_RUNTIME_DIR was not set, defaulting to" << runtimeDir;
		}

		this->mBaseRunDir = QDir(runtimeDir);
		this->mBaseRunDir = QDir(this->mBaseRunDir.filePath("quickshell"));
		qCDebug(logPaths) << "Initialized base runtime path:" << this->mBaseRunDir.path();

		if (!this->mBaseRunDir.mkpath(".")) {
			qCCritical(logPaths) << "Could not create base runtime directory at"
			                     << this->mBaseRunDir.path();

			this->baseRunState = DirState::Failed;
		} else {
			this->baseRunState = DirState::Ready;
		}
	}

	if (this->baseRunState == DirState::Failed) return nullptr;
	else return &this->mBaseRunDir;
}

QDir* QsPaths::runDir() {
	if (this->runState == DirState::Unknown) {
		if (auto* baseRunDir = this->baseRunDir()) {
			this->mRunDir = QDir(baseRunDir->filePath(this->shellId));

			qCDebug(logPaths) << "Initialized runtime path:" << this->mRunDir.path();

			if (!this->mRunDir.mkpath(".")) {
				qCCritical(logPaths) << "Could not create runtime directory at" << this->mRunDir.path();
				this->runState = DirState::Failed;
			} else {
				this->runState = DirState::Ready;
			}
		} else {
			qCCritical(logPaths) << "Could not create shell runtime path as it was not possible to "
			                        "create the base runtime path.";

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
			    runtimeDir->filePath(QString("run-%1").arg(qs::Common::LAUNCH_TIME.toMSecsSinceEpoch()));

			qCDebug(logPaths) << "Initialized instance runtime path:" << this->mInstanceRunDir.path();

			if (!this->mInstanceRunDir.mkpath(".")) {
				qCCritical(logPaths) << "Could not create instance runtime directory at"
				                     << this->mInstanceRunDir.path();
				this->instanceRunState = DirState::Failed;
			} else {
				this->instanceRunState = DirState::Ready;
			}
		}
	}

	if (this->runState == DirState::Failed) return nullptr;
	else return &this->mInstanceRunDir;
}

void QsPaths::linkPidRunDir() {
	if (auto* runDir = this->instanceRunDir()) {
		auto pidDir = QDir(this->baseRunDir()->filePath("by-pid"));

		if (!pidDir.mkpath(".")) {
			qCCritical(logPaths) << "Could not create PID symlink directory.";
			return;
		}

		auto pidPath = pidDir.filePath(QString::number(getpid()));

		QFile::remove(pidPath);
		auto r = symlinkat(runDir->filesystemCanonicalPath().c_str(), 0, pidPath.toStdString().c_str());

		if (r != 0) {
			qCCritical(logPaths).nospace()
			    << "Could not create PID symlink to " << runDir->path() << " at " << pidPath
			    << " with error code " << errno << ": " << qt_error_string();
		} else {
			qCDebug(logPaths) << "Created PID symlink" << pidPath << "to instance runtime path"
			                  << runDir->path();
		}
	} else {
		qCCritical(logPaths) << "Could not create PID symlink to runtime directory, as the runtime "
		                        "directory could not be created.";
	}
}
