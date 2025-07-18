#include "toolsupport.hpp"
#include <cerrno>

#include <fcntl.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qqmlengine.h>
#include <qtenvironmentvariables.h>

#include "logcat.hpp"
#include "paths.hpp"
#include "scan.hpp"

namespace qs::core {

namespace {
QS_LOGGING_CATEGORY(logTooling, "quickshell.tooling", QtWarningMsg);
}

bool QmlToolingSupport::updateTooling(const QDir& configRoot, QmlScanner& scanner) {
	auto* vfs = QsPaths::instance()->shellVfsDir();

	if (!vfs) {
		qCCritical(logTooling) << "Tooling dir could not be created";
		return false;
	}

	if (!QmlToolingSupport::lockTooling()) {
		return false;
	}

	if (!QmlToolingSupport::updateQmllsConfig(configRoot, false)) {
		QDir(vfs->filePath("qs")).removeRecursively();
		return false;
	}

	QmlToolingSupport::updateToolingFs(scanner, configRoot, vfs->filePath("qs"));
	return true;
}

bool QmlToolingSupport::lockTooling() {
	if (QmlToolingSupport::toolingLock) return true;

	auto lockPath = QsPaths::instance()->shellVfsDir()->filePath("tooling.lock");
	auto* file = new QFile(lockPath);

	if (!file->open(QFile::WriteOnly)) {
		qCCritical(logTooling) << "Could not open tooling lock for write";
		return false;
	}

	auto lock = flock {
	    .l_type = F_WRLCK,
	    .l_whence = SEEK_SET, // NOLINT (fcntl.h??)
	    .l_start = 0,
	    .l_len = 0,
	    .l_pid = 0,
	};

	if (fcntl(file->handle(), F_SETLK, &lock) == 0) {
		qCInfo(logTooling) << "Acquired tooling support lock";
		QmlToolingSupport::toolingLock = file;
		return true;
	} else if (errno == EACCES || errno == EAGAIN) {
		qCInfo(logTooling) << "Tooling support locked by another instance";
		return false;
	} else {
		qCCritical(logTooling).nospace() << "Could not create tooling lock at " << lockPath
		                                 << " with error code " << errno << ": " << qt_error_string();
		return false;
	}
}

QString QmlToolingSupport::getQmllsConfig() {
	static auto config = []() {
		// We can't replicate the algorithm used to create the import path list as it can have distro
		// specific patches, e.g. nixos.
		auto importPaths = QQmlEngine().importPathList();
		importPaths.removeIf([](const QString& path) { return path.startsWith("qrc:"); });

		auto vfsPath = QsPaths::instance()->shellVfsDir()->path();
		auto importPathsStr = importPaths.join(u':');

		QString qmllsConfig;
		auto print = QDebug(&qmllsConfig).nospace();
		print << "[General]\nno-cmake-calls=true\nbuildDir=" << vfsPath
		      << "\nimportPaths=" << importPathsStr << '\n';

		return qmllsConfig;
	}();

	return config;
}

bool QmlToolingSupport::updateQmllsConfig(const QDir& configRoot, bool create) {
	auto shellConfigPath = configRoot.filePath(".qmlls.ini");
	auto vfsConfigPath = QsPaths::instance()->shellVfsDir()->filePath(".qmlls.ini");

	auto shellFileInfo = QFileInfo(shellConfigPath);
	if (!create && !shellFileInfo.exists() && !shellFileInfo.isSymLink()) {
		if (QmlToolingSupport::toolingEnabled) {
			qInfo() << "QML tooling support disabled";
			QmlToolingSupport::toolingEnabled = false;
		} else {
			qCInfo(logTooling) << "Not enabling QML tooling support, qmlls.ini is missing at path"
			                   << shellConfigPath;
		}

		QFile::remove(vfsConfigPath);
		return false;
	}

	auto vfsFile = QFile(vfsConfigPath);

	if (!vfsFile.open(QFile::ReadWrite | QFile::Text)) {
		qCCritical(logTooling) << "Failed to create qmlls config in vfs";
		return false;
	}

	auto config = QmlToolingSupport::getQmllsConfig();

	if (vfsFile.readAll() != config) {
		if (!vfsFile.resize(0) || !vfsFile.write(config.toUtf8())) {
			qCCritical(logTooling) << "Failed to write qmlls config in vfs";
			return false;
		}

		qCDebug(logTooling) << "Wrote qmlls config in vfs";
	}

	if (!shellFileInfo.isSymLink() || shellFileInfo.symLinkTarget() != vfsConfigPath) {
		QFile::remove(shellConfigPath);

		if (!QFile::link(vfsConfigPath, shellConfigPath)) {
			qCCritical(logTooling) << "Failed to create qmlls config symlink";
			return false;
		}

		qCDebug(logTooling) << "Created qmlls config symlink";
	}

	if (!QmlToolingSupport::toolingEnabled) {
		qInfo() << "QML tooling support enabled";
		QmlToolingSupport::toolingEnabled = true;
	}

	return true;
}

void QmlToolingSupport::updateToolingFs(
    QmlScanner& scanner,
    const QDir& scanDir,
    const QDir& linkDir
) {
	QList<QString> files;
	QSet<QString> subdirs;

	auto scanPath = scanDir.path();

	linkDir.mkpath(".");

	for (auto& path: scanner.scannedFiles) {
		if (path.length() < scanPath.length() + 1 || !path.startsWith(scanPath)) continue;
		auto name = path.sliced(scanPath.length() + 1);

		if (name.contains('/')) {
			auto dirname = name.first(name.indexOf('/'));
			subdirs.insert(dirname);
			continue;
		}

		auto fileInfo = QFileInfo(path);
		if (!fileInfo.isFile()) continue;

		auto spath = linkDir.filePath(name);
		auto sFileInfo = QFileInfo(spath);

		if (!sFileInfo.isSymLink() || sFileInfo.symLinkTarget() != path) {
			QFile::remove(spath);

			if (QFile::link(path, spath)) {
				qCDebug(logTooling) << "Created symlink to" << path << "at" << spath;
				files.append(spath);
			} else {
				qCCritical(logTooling) << "Could not create symlink to" << path << "at" << spath;
			}
		} else {
			files.append(spath);
		}
	}

	for (auto [path, text]: scanner.fileIntercepts.asKeyValueRange()) {
		if (path.length() < scanPath.length() + 1 || !path.startsWith(scanPath)) continue;
		auto name = path.sliced(scanPath.length() + 1);

		if (name.contains('/')) {
			auto dirname = name.first(name.indexOf('/'));
			subdirs.insert(dirname);
			continue;
		}

		auto spath = linkDir.filePath(name);
		auto file = QFile(spath);
		if (!file.open(QFile::ReadWrite | QFile::Text)) {
			qCCritical(logTooling) << "Failed to open injected file" << spath;
			continue;
		}

		if (file.readAll() == text) {
			files.append(spath);
			continue;
		}

		if (file.resize(0) && file.write(text.toUtf8())) {
			files.append(spath);
			qCDebug(logTooling) << "Wrote injected file" << spath;
		} else {
			qCCritical(logTooling) << "Failed to write injected file" << spath;
		}
	}

	for (auto& name: linkDir.entryList(QDir::Files | QDir::System)) { // System = broken symlinks
		auto path = linkDir.filePath(name);

		if (!files.contains(path)) {
			if (QFile::remove(path)) qCDebug(logTooling) << "Removed old file at" << path;
			else qCWarning(logTooling) << "Failed to remove old file at" << path;
		}
	}

	for (const auto& subdir: subdirs) {
		QmlToolingSupport::updateToolingFs(scanner, scanDir.filePath(subdir), linkDir.filePath(subdir));
	}
}

} // namespace qs::core
