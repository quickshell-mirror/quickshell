#include "desktopentrymonitor.hpp"

#include <qdir.h>
#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtenvironmentvariables.h>

#include "desktoputils.hpp"
#include "logcat.hpp"

namespace {
QS_LOGGING_CATEGORY(logDesktopMonitor, "quickshell.desktopentrymonitor", QtWarningMsg);
}

DesktopEntryMonitor::DesktopEntryMonitor(QObject* parent): QObject(parent) {
	this->watcher = new QFileSystemWatcher(this);
	this->debounceTimer = new QTimer(this);
	this->debounceTimer->setSingleShot(true);
	this->debounceTimer->setInterval(500); // 500ms debounce

	// Initialize XDG desktop paths
	this->initializeDesktopPaths();

	// Setup connections
	connect(
	    this->watcher,
	    &QFileSystemWatcher::directoryChanged,
	    this,
	    &DesktopEntryMonitor::onDirectoryChanged
	);
	connect(
	    this->watcher,
	    &QFileSystemWatcher::fileChanged,
	    this,
	    &DesktopEntryMonitor::onFileChanged
	);
	connect(this->debounceTimer, &QTimer::timeout, this, &DesktopEntryMonitor::processChanges);

	// Start monitoring
	this->startMonitoring();
}

void DesktopEntryMonitor::initializeDesktopPaths() {
	this->desktopPaths = DesktopUtils::getDesktopDirectories();
}

void DesktopEntryMonitor::startMonitoring() {
	for (const QString& path: this->desktopPaths) {
		if (QDir(path).exists()) {
			qCDebug(logDesktopMonitor) << "Monitoring desktop entry path:" << path;
			this->addDirectoryHierarchy(path);
		}
	}
}

void DesktopEntryMonitor::addDirectoryHierarchy(const QString& dirPath) {
	this->scanAndWatch(dirPath);
}

void DesktopEntryMonitor::scanAndWatch(const QString& dirPath) {
	QDir dir(dirPath);
	if (!dir.exists()) return;

	// Add directory to watcher
	if (this->watcher->addPath(dirPath)) {
		qCDebug(logDesktopMonitor) << "Added directory to watcher:" << dirPath;
	}

	// Add .desktop files
	for (const auto& entry: dir.entryInfoList({"*.desktop"}, QDir::Files)) {
		auto path = entry.absoluteFilePath();
		if (!this->watchedFiles.contains(path)) {
			if (this->watcher->addPath(path)) {
				this->fileTimestamps[path] = entry.lastModified();
				this->watchedFiles.insert(path);
				qCDebug(logDesktopMonitor) << "Monitoring file:" << path;
			}
		}
	}

	// Recurse into subdirs
	for (const auto& sub: dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		this->scanAndWatch(dir.absoluteFilePath(sub));
	}
}

void DesktopEntryMonitor::onDirectoryChanged(const QString& path) {
	qCDebug(logDesktopMonitor) << "Directory changed:" << path;
	QDir dir(path);

	// Check if directory still exists - handle removal/unmounting
	if (!dir.exists()) {
		qCDebug(logDesktopMonitor) << "Directory no longer exists, cleaning up:" << path;
		this->watcher->removePath(path);

		// Remove all watched files from this directory and its subdirectories
		for (auto it = this->watchedFiles.begin(); it != this->watchedFiles.end();) {
			const QString& watchedFile = *it;
			if (watchedFile.startsWith(path + "/") || watchedFile == path) {
				this->watcher->removePath(watchedFile);
				this->fileTimestamps.remove(watchedFile);
				this->queueChange(ChangeEvent::Removed, watchedFile);
				qCDebug(logDesktopMonitor) << "Removed file due to directory deletion:" << watchedFile;
				it = this->watchedFiles.erase(it);
			} else {
				++it;
			}
		}
		return;
	}

	QList<QString> currentFiles = dir.entryList({"*.desktop"}, QDir::Files);

	// Check for new files
	for (const QString& file: currentFiles) {
		QString fullPath = dir.absoluteFilePath(file);
		if (!this->watchedFiles.contains(fullPath)) {
			if (this->watcher->addPath(fullPath)) {
				this->fileTimestamps[fullPath] = QFileInfo(fullPath).lastModified();
				this->watchedFiles.insert(fullPath);
				this->queueChange(ChangeEvent::Added, fullPath);
				qCDebug(logDesktopMonitor) << "New desktop file detected:" << fullPath;
			}
		}
	}

	// Check for deleted files
	for (auto it = this->watchedFiles.begin(); it != this->watchedFiles.end();) {
		const QString& watchedFile = *it;
		if (QFileInfo(watchedFile).dir().absolutePath() == path && !QFile::exists(watchedFile)) {
			this->watcher->removePath(watchedFile);
			this->fileTimestamps.remove(watchedFile);
			this->queueChange(ChangeEvent::Removed, watchedFile);
			qCDebug(logDesktopMonitor) << "Desktop file removed:" << watchedFile;
			it = this->watchedFiles.erase(it);
		} else {
			++it;
		}
	}

	// Check for new subdirectories
	QList<QString> subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString& subdir: subdirs) {
		QString subdirPath = dir.absoluteFilePath(subdir);
		if (!this->watcher->directories().contains(subdirPath)) {
			this->scanAndWatch(subdirPath);
		}
	}
}

void DesktopEntryMonitor::onFileChanged(const QString& path) {
	if (QFileInfo(path).suffix().toLower() != "desktop") return;

	QFileInfo info(path);
	if (info.exists()) {
		QDateTime currentModified = info.lastModified();
		if (this->fileTimestamps.value(path) != currentModified) {
			this->fileTimestamps[path] = currentModified;
			this->queueChange(ChangeEvent::Modified, path);
			qCDebug(logDesktopMonitor) << "Desktop file modified:" << path;
		}
	} else {
		// File was deleted
		this->watcher->removePath(path);
		this->fileTimestamps.remove(path);
		this->watchedFiles.remove(path);
		this->queueChange(ChangeEvent::Removed, path);
		qCDebug(logDesktopMonitor) << "Desktop file removed:" << path;
	}
}

void DesktopEntryMonitor::queueChange(ChangeEvent event, const QString& path) {
	this->pendingChanges.insert(path, event);
	this->debounceTimer->start();
}

void DesktopEntryMonitor::processChanges() {
	if (this->pendingChanges.isEmpty()) return;

	qCDebug(logDesktopMonitor) << "Processing" << this->pendingChanges.size() << "pending changes";

	QHash<QString, ChangeEvent> changes = this->pendingChanges;
	this->pendingChanges.clear();

	emit desktopEntriesChanged(changes);
}