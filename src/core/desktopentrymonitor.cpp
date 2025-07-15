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
		// Directory removal will be handled by full rescan
		this->queueChange(ChangeEvent::Modified, path); // Trigger full rescan
		return;
	}

	// Check for new subdirectories
	QList<QString> subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString& subdir: subdirs) {
		QString subdirPath = dir.absoluteFilePath(subdir);
		if (!this->watcher->directories().contains(subdirPath)) {
			this->scanAndWatch(subdirPath);
		}
	}

	// Queue a change to trigger full rescan of all desktop paths
	this->queueChange(ChangeEvent::Modified, path);
}

void DesktopEntryMonitor::queueChange(ChangeEvent event, const QString& path) {
	this->pendingChanges.insert(path, event);
	this->debounceTimer->start();
}

void DesktopEntryMonitor::processChanges() {
	if (this->pendingChanges.isEmpty()) return;

	qCDebug(logDesktopMonitor) << "Processing directory changes, triggering full rescan";

	// Clear pending changes since we're doing a full rescan
	this->pendingChanges.clear();

	// Emit with empty hash to signal full rescan needed
	QHash<QString, ChangeEvent> emptyChanges;
	emit desktopEntriesChanged(emptyChanges);
}