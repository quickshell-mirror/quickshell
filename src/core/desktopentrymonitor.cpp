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
QS_LOGGING_CATEGORY(logDesktopMonitor, "quickshell.desktopmonitor", QtWarningMsg);
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
			this->addDirectoryRecursive(path);
		}
	}
}

void DesktopEntryMonitor::addDirectoryRecursive(const QString& dirPath) {
	QDir dir(dirPath);
	if (!dir.exists()) return;

	// Add root directory to watcher
	if (this->watcher->addPath(dirPath)) {
		qCDebug(logDesktopMonitor) << "Added directory to watcher:" << dirPath;
	}

	QDirIterator it(
	    dirPath,
	    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
	    QDirIterator::Subdirectories
	);

	while (it.hasNext()) {
		QString path = it.next();
		QFileInfo info(path);

		if (info.isDir()) {
			if (this->watcher->addPath(path)) {
				qCDebug(logDesktopMonitor) << "Added directory to watcher:" << path;
			}
		} else if (path.endsWith(".desktop")) {
			if (this->watcher->addPath(path)) {
				this->fileTimestamps[path] = info.lastModified();
				qCDebug(logDesktopMonitor) << "Monitoring file:" << path;
			}
		}
	}
}

void DesktopEntryMonitor::onDirectoryChanged(const QString& path) {
	qCDebug(logDesktopMonitor) << "Directory changed:" << path;
	QDir dir(path);
	QList<QString> currentFiles = dir.entryList({"*.desktop"}, QDir::Files);

	// Check for new files
	for (const QString& file: currentFiles) {
		QString fullPath = dir.absoluteFilePath(file);
		if (!this->watcher->files().contains(fullPath)) {
			if (this->watcher->addPath(fullPath)) {
				this->fileTimestamps[fullPath] = QFileInfo(fullPath).lastModified();
				this->queueChange(ChangeEvent::Added, fullPath);
				qCDebug(logDesktopMonitor) << "New desktop file detected:" << fullPath;
			}
		}
	}

	// Check for deleted files
	QList<QString> watchedFiles = this->watcher->files();
	for (const QString& watchedFile: watchedFiles) {
		if (QFileInfo(watchedFile).dir().absolutePath() == path && !QFile::exists(watchedFile)) {
			this->watcher->removePath(watchedFile);
			this->fileTimestamps.remove(watchedFile);
			this->queueChange(ChangeEvent::Removed, watchedFile);
			qCDebug(logDesktopMonitor) << "Desktop file removed:" << watchedFile;
		}
	}

	// Check for new subdirectories
	QList<QString> subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString& subdir: subdirs) {
		QString subdirPath = dir.absoluteFilePath(subdir);
		if (!this->watcher->directories().contains(subdirPath)) {
			// Add the new subdirectory and all its contents iteratively
			if (this->watcher->addPath(subdirPath)) {
				qCDebug(logDesktopMonitor) << "Added new directory to watcher:" << subdirPath;
			}

			QDirIterator it(
			    subdirPath,
			    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
			    QDirIterator::Subdirectories
			);

			while (it.hasNext()) {
				QString path = it.next();
				QFileInfo info(path);

				if (info.isDir()) {
					if (this->watcher->addPath(path)) {
						qCDebug(logDesktopMonitor) << "Added directory to watcher:" << path;
					}
				} else if (path.endsWith(".desktop")) {
					if (this->watcher->addPath(path)) {
						this->fileTimestamps[path] = info.lastModified();
						qCDebug(logDesktopMonitor) << "Monitoring file:" << path;
					}
				}
			}
		}
	}
}

void DesktopEntryMonitor::onFileChanged(const QString& path) {
	if (!path.endsWith(".desktop")) return;

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