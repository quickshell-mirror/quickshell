#include "toolsupport.hpp"
#include <algorithm>
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
#include <qsavefile.h>
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

bool QmlToolingSupport::updateTooling(const QDir& configRoot, bool keepMirror) {
	auto* vfs = QsPaths::instance()->shellVfsDir();

	if (!vfs) {
		qCCritical(logTooling) << "Tooling dir could not be created";
		return false;
	}

	if (!QmlToolingSupport::lockTooling()) {
		return false;
	}

	if (QmlToolingSupport::updateQmllsConfig(configRoot, false)) return true;
	if (!keepMirror) QmlToolingSupport::removeMirrorEntry(vfs->filePath("qs"));
	return false;
}

QString QmlToolingSupport::updateMirror(
    const QDir& configRoot,
    const QmlScanner& scanner,
    bool copyDocuments
) {
	auto* vfs = QsPaths::instance()->shellVfsDir();
	if (!vfs) return QString();

	// QQmlTypeLoader::addImportPath canonicalizes, so urls built from this must match
	auto vfsPath = vfs->canonicalPath();
	if (vfsPath.isEmpty()) return QString();

	auto target = QDir(vfsPath % "/qs");
	if (!QmlToolingSupport::mirrorDir(scanner, configRoot, target, copyDocuments)) return QString();

	return vfsPath;
}

bool QmlToolingSupport::mirrorDir(
    const QmlScanner& scanner,
    const QDir& source,
    const QDir& target,
    bool copyDocuments
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

		auto path = source.filePath(name);
		auto entryTarget = target.filePath(name);

		if (!QmlToolingSupport::mirrorEntry(scanner, path, entryTarget, copyDocuments)) return false;
	}

	for (const auto& name: target.entryList(filters)) {
		if (!names.contains(name)) QmlToolingSupport::removeMirrorEntry(target.filePath(name));
	}

	return true;
}

bool QmlToolingSupport::mirrorEntry(
    const QmlScanner& scanner,
    const QString& path,
    const QString& target,
    bool copyDocuments
) {
	auto info = QFileInfo(path);
	auto name = info.fileName();

	// hidden entries can't be modules
	if (name.startsWith('.')) return QmlToolingSupport::linkMirrorEntry(path, target);

	if (info.isDir()) {
		// symlinked dirs can loop, so only the ones the scanner produced files under are entered
		auto prefix = path % '/';
		auto scanned = std::any_of(
		    scanner.fileIntercepts.keyBegin(),
		    scanner.fileIntercepts.keyEnd(),
		    [&](const QString& key) { return key.startsWith(prefix); }
		);

		if (info.isSymLink() && !scanned) return QmlToolingSupport::linkMirrorEntry(path, target);
		return QmlToolingSupport::mirrorDir(scanner, QDir(path), QDir(target), copyDocuments);
	}

	auto isDocument = name.endsWith(".qml") || name.endsWith(".js") || name.endsWith(".mjs");
	if (!copyDocuments || !isDocument) return QmlToolingSupport::linkMirrorEntry(path, target);

	auto file = QFile(path);

	if (!file.open(QFile::ReadOnly)) {
		qCCritical(logTooling) << "Could not read" << path;
		return false;
	}

	return QmlToolingSupport::writeMirrorFile(target, file.readAll());
}

bool QmlToolingSupport::writeMirrorFile(const QString& path, const QByteArray& data) {
	auto info = QFileInfo(path);
	if (info.isSymLink() || (info.exists() && !info.isFile())) {
		QmlToolingSupport::removeMirrorEntry(path);
	}

	auto current = QFile(path);
	if (current.open(QFile::ReadOnly) && current.readAll() == data) return true;
	current.close();

	auto save = QSaveFile(path);

	if (!save.open(QFile::WriteOnly) || save.write(data) != data.length() || !save.commit()) {
		qCCritical(logTooling) << "Failed to write mirror file" << path;
		return false;
	}

	// The disk cache compares source mtimes (QV4::CompiledData::Unit::verifyHeader), which
	// are all 1 in the nix store, so the mtime comes from the content. 2000..2030, never 0.
	auto hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
	auto secs = 946684800 + qFromBigEndian<quint32>(hash.constData()) % 946684800;
	auto file = QFile(path);

	if (!file.open(QFile::ReadWrite)
	    || !file.setFileTime(QDateTime::fromSecsSinceEpoch(secs), QFile::FileModificationTime))
	{
		qCCritical(logTooling) << "Failed to set mirror file time on" << path;
		return false;
	}

	qCDebug(logTooling) << "Wrote mirror file" << path;
	return true;
}

bool QmlToolingSupport::linkMirrorEntry(const QString& path, const QString& target) {
	auto info = QFileInfo(target);
	if (info.isSymLink() && info.symLinkTarget() == path) return true;

	QmlToolingSupport::removeMirrorEntry(target);

	if (!QFile::link(path, target)) {
		qCCritical(logTooling) << "Could not create symlink to" << path << "at" << target;
		return false;
	}

	qCDebug(logTooling) << "Created symlink to" << path << "at" << target;
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
