#include "toolsupport.hpp"

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
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

	if (!QmlToolingSupport::updateQmllsConfig(configRoot, false)) {
		QDir(vfs->filePath("qs")).removeRecursively();
		return false;
	}

	QmlToolingSupport::updateToolingFs(scanner, configRoot, vfs->filePath("qs"));
	return true;
}

QString QmlToolingSupport::getQmllsConfig() {
	static auto config = []() {
		QList<QString> importPaths;

		auto addPaths = [&](const QList<QString>& paths) {
			for (const auto& path: paths) {
				if (!importPaths.contains(path)) importPaths.append(path);
			}
		};

		addPaths(qEnvironmentVariable("QML_IMPORT_PATH").split(u':', Qt::SkipEmptyParts));
		addPaths(qEnvironmentVariable("QML2_IMPORT_PATH").split(u':', Qt::SkipEmptyParts));

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
	if (!create && !shellFileInfo.exists()) {
		if (QmlToolingSupport::toolingEnabled) {
			qInfo() << "QML tooling support disabled";
			QmlToolingSupport::toolingEnabled = false;
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
