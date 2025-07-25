#include "desktopentrymonitor.hpp"

#include <qdir.h>
#include <qdiriterator.h>
#include <qfileinfo.h>

#include "desktopentry.hpp"

DesktopEntryMonitor::DesktopEntryMonitor(QObject* parent): QObject(parent) {
	this->watcher = new QFileSystemWatcher(this);
	this->debounceTimer = new QTimer(this);
	this->debounceTimer->setSingleShot(true);
	this->debounceTimer->setInterval(100);

	QObject::connect(
	    this->watcher,
	    &QFileSystemWatcher::directoryChanged,
	    this,
	    &DesktopEntryMonitor::onDirectoryChanged
	);
	QObject::connect(
	    this->debounceTimer,
	    &QTimer::timeout,
	    this,
	    &DesktopEntryMonitor::processChanges
	);

	this->startMonitoring();
}

void DesktopEntryMonitor::startMonitoring() {
	for (const auto& path: DesktopEntryManager::desktopPaths()) {
		if (QDir(path).exists()) {
			this->scanAndWatch(path);
		}
	}
}

void DesktopEntryMonitor::scanAndWatch(const QString& dirPath) {
	auto dir = QDir(dirPath);
	if (!dir.exists()) return;

	this->watcher->addPath(dirPath);

	auto subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const auto& subdir: subdirs) {
		this->watcher->addPath(subdir.absoluteFilePath());
	}
}

void DesktopEntryMonitor::onDirectoryChanged(const QString& path) {
	this->scanAndWatch(path);

	if (!this->rescanPending) {
		this->rescanPending = true;
		this->debounceTimer->start();
	}
}

void DesktopEntryMonitor::processChanges() {
	if (!this->rescanPending) return;

	this->rescanPending = false;

	emit desktopEntriesChanged();
}