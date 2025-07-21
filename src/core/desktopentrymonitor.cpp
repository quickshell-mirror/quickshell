#include "desktopentrymonitor.hpp"

#include <qdir.h>
#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qtenvironmentvariables.h>

DesktopEntryMonitor::DesktopEntryMonitor(QObject* parent): QObject(parent) {
	this->watcher = new QFileSystemWatcher(this);
	this->debounceTimer = new QTimer(this);
	this->debounceTimer->setSingleShot(true);
	this->debounceTimer->setInterval(100);

	this->initializeDesktopPaths();
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

QStringList DesktopEntryMonitor::getDesktopDirectories() const { return this->desktopPaths; }

void DesktopEntryMonitor::initializeDesktopPaths() {
	auto dataPaths = QList<QString>();

	auto dataHome = qEnvironmentVariable("XDG_DATA_HOME");
	if (dataHome.isEmpty()) {
		if (qEnvironmentVariableIsSet("HOME")) {
			dataHome = qEnvironmentVariable("HOME") + "/.local/share";
		}
	}
	if (!dataHome.isEmpty()) {
		dataPaths.append(dataHome + "/applications");
	}

	auto dataDirs = qEnvironmentVariable("XDG_DATA_DIRS");
	if (dataDirs.isEmpty()) {
		dataDirs = "/usr/local/share:/usr/share";
	}

	for (const auto& dir: dataDirs.split(':', Qt::SkipEmptyParts)) {
		if (!dir.isEmpty()) {
			dataPaths.append(dir + "/applications");
		}
	}

	this->desktopPaths = dataPaths;
}

void DesktopEntryMonitor::startMonitoring() {
	for (const auto& path: this->desktopPaths) {
		if (QDir(path).exists()) {
			this->scanAndWatch(path);
		}
	}
}

void DesktopEntryMonitor::scanAndWatch(const QString& dirPath) {
	auto dir = QDir(dirPath);
	if (!dir.exists()) return;

	if (!this->watcher->directories().contains(dirPath)) {
		this->watcher->addPath(dirPath);
	}

	auto subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const auto& subdir: subdirs) {
		auto subdirPath = subdir.absoluteFilePath();
		if (!this->watcher->directories().contains(subdirPath)) {
			this->watcher->addPath(subdirPath);
		}
	}
}

void DesktopEntryMonitor::onDirectoryChanged(const QString& path) {
	auto dir = QDir(path);

	if (!dir.exists()) {
		this->watcher->removePath(path);
	} else {
		this->scanAndWatch(path);
	}

	if (!this->rescanPending) {
		this->rescanPending = true;
		this->debounceTimer->start();
	}
}

void DesktopEntryMonitor::processChanges() {
	if (!this->rescanPending) return;

	this->rescanPending = false;

	auto watchedPaths = this->watcher->directories();
	for (const auto& path: watchedPaths) {
		if (!QDir(path).exists()) {
			this->watcher->removePath(path);
		}
	}

	emit desktopEntriesChanged();
}