#include "toolsupport.hpp"
#include <cerrno>

#include <fcntl.h>
#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qcryptographichash.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qdir.h>
#include <qendian.h>
#include <qfile.h>
#include <qfiledevice.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qqmlengine.h>
#include <qset.h>
#include <qtenvironmentvariables.h>
#include <qtypes.h>

#include "logcat.hpp"
#include "paths.hpp"
#include "scan.hpp"

namespace qs::core {

namespace {
QS_LOGGING_CATEGORY(logTooling, "quickshell.tooling", QtWarningMsg);
}

bool QmlToolingSupport::updateTooling(const QDir& configRoot) {
	if (!QsPaths::instance()->shellVfsDir()) {
		qCCritical(logTooling) << "Tooling dir could not be created";
		return false;
	}

	if (!QmlToolingSupport::lockTooling()) {
		return false;
	}

	return QmlToolingSupport::updateQmllsConfig(configRoot, false);
}

QString QmlToolingSupport::updateMirror(const QDir& configRoot, const QmlScanner& scanner) {
	auto* vfs = QsPaths::instance()->shellVfsDir();
	if (!vfs) return QString();

	// the engine canonicalizes import paths, so every url derived from this one must match
	auto vfsPath = vfs->canonicalPath();
	if (vfsPath.isEmpty()) return QString();
	if (!QmlToolingSupport::mirrorDir(scanner, configRoot, QDir(vfsPath % "/qs"))) return QString();

	return vfsPath;
}

bool QmlToolingSupport::mirrorDir(
    const QmlScanner& scanner,
    const QDir& source,
    const QDir& target
) {
	auto targetInfo = QFileInfo(target.path());
	if (targetInfo.isSymLink() || (targetInfo.exists() && !targetInfo.isDir())) {
		QmlToolingSupport::removeMirrorEntry(target.path());
	}

	if (!target.mkpath(".")) {
		qCCritical(logTooling) << "Could not create mirror dir at" << target.path();
		return false;
	}

	const QString prefix = source.path() % '/';
	QSet<QString> names;

	for (auto [path, text]: scanner.fileIntercepts.asKeyValueRange()) {
		if (!path.startsWith(prefix)) continue;

		auto name = path.sliced(prefix.length());
		if (name.contains('/')) continue;

		names.insert(name);
		if (!QmlToolingSupport::writeMirrorFile(target.filePath(name), text.toUtf8())) return false;
	}

	auto filters = QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot;

	for (const auto& name: source.entryList(filters)) {
		if (names.contains(name)) continue;
		names.insert(name);

		if (!QmlToolingSupport::mirrorEntry(scanner, source.filePath(name), target.filePath(name))) {
			return false;
		}
	}

	for (const auto& name: target.entryList(filters)) {
		if (!names.contains(name)) QmlToolingSupport::removeMirrorEntry(target.filePath(name));
	}

	return true;
}

bool QmlToolingSupport::mirrorEntry(
    const QmlScanner& scanner,
    const QString& path,
    const QString& target
) {
	auto info = QFileInfo(path);
	auto name = info.fileName();

	// hidden entries can't be modules and .git alone would be thousands of links
	if (name.startsWith('.')) return QmlToolingSupport::linkMirrorEntry(path, target);
	if (info.isDir()) return QmlToolingSupport::mirrorDir(scanner, QDir(path), QDir(target));

	auto isDocument = name.endsWith(".qml") || name.endsWith(".js") || name.endsWith(".mjs");
	if (!isDocument) return QmlToolingSupport::linkMirrorEntry(path, target);

	auto file = QFile(path);

	if (!file.open(QFile::ReadOnly)) {
		qCWarning(logTooling) << "Could not read" << path << "for the mirror, linking it instead";
		return QmlToolingSupport::linkMirrorEntry(path, target);
	}

	return QmlToolingSupport::writeMirrorFile(target, file.readAll());
}

bool QmlToolingSupport::writeMirrorFile(const QString& path, const QByteArray& data) {
	auto info = QFileInfo(path);
	if (info.isSymLink() || (info.exists() && !info.isFile())) {
		QmlToolingSupport::removeMirrorEntry(path);
	}

	auto file = QFile(path);

	if (!file.open(QFile::ReadWrite)) {
		qCCritical(logTooling) << "Failed to open mirror file" << path;
		return false;
	}

	if (file.readAll() != data) {
		if (!file.resize(0) || file.write(data) != data.length() || !file.flush()) {
			qCCritical(logTooling) << "Failed to write mirror file" << path;
			return false;
		}

		qCDebug(logTooling) << "Wrote mirror file" << path;
	}

	// Qt validates cache entries by source mtime, which is always 1 in the nix store, so the
	// mtime is derived from content instead. Spans 2000..2030 as a zero stamp disables the check.
	auto hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
	auto secs = 946684800 + qFromBigEndian<quint32>(hash.constData()) % 946684800;

	if (!file.setFileTime(QDateTime::fromSecsSinceEpoch(secs), QFile::FileModificationTime)) {
		qCCritical(logTooling) << "Failed to set mirror file time on" << path;
		return false;
	}

	return true;
}

bool QmlToolingSupport::linkMirrorEntry(const QString& path, const QString& linkPath) {
	auto info = QFileInfo(linkPath);
	if (info.isSymLink() && info.symLinkTarget() == path) return true;

	QmlToolingSupport::removeMirrorEntry(linkPath);

	if (!QFile::link(path, linkPath)) {
		qCCritical(logTooling) << "Could not create symlink to" << path << "at" << linkPath;
		return false;
	}

	qCDebug(logTooling) << "Created symlink to" << path << "at" << linkPath;
	return true;
}

void QmlToolingSupport::removeMirrorEntry(const QString& path) {
	auto info = QFileInfo(path);
	if (!info.exists() && !info.isSymLink()) return;

	// isDir follows symlinks, and everything behind one is the real config
	auto ok =
	    info.isDir() && !info.isSymLink() ? QDir(path).removeRecursively() : QFile::remove(path);
	if (!ok) qCWarning(logTooling) << "Failed to remove old file at" << path;
}

bool QmlToolingSupport::lockTooling() {
	if (QmlToolingSupport::toolingLock) return true;

	auto lockPath = QsPaths::instance()->shellVfsDir()->filePath("tooling.lock");
	auto* file = new QFile(lockPath);

	if (!file->open(QFile::WriteOnly)) {
		qCCritical(logTooling) << "Could not open tooling lock for write";
		return false;
	}

	struct flock lock = {
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

} // namespace qs::core
