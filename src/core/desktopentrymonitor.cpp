#include "desktopentrymonitor.hpp"

#include <qdir.h>
#include <qfileinfo.h>
#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "desktopentry.hpp"

namespace {
void addPathAndParents(QFileSystemWatcher& watcher, const QString& path) {
	watcher.addPath(path);

	auto p = QFileInfo(path).absolutePath();
	while (!p.isEmpty()) {
		watcher.addPath(p);
		const auto parent = QFileInfo(p).dir().absolutePath();
		if (parent == p) break;
		p = parent;
	}
}
} // namespace

DesktopEntryMonitor::DesktopEntryMonitor(QObject* parent): QObject(parent) {
	this->debounceTimer.setSingleShot(true);
	this->debounceTimer.setInterval(100);

	QObject::connect(
	    &this->watcher,
	    &QFileSystemWatcher::directoryChanged,
	    this,
	    &DesktopEntryMonitor::onWatchedPathChanged
	);
	QObject::connect(
	    &this->watcher,
	    &QFileSystemWatcher::fileChanged,
	    this,
	    &DesktopEntryMonitor::onWatchedPathChanged
	);
	QObject::connect(
	    &this->debounceTimer,
	    &QTimer::timeout,
	    this,
	    &DesktopEntryMonitor::processChanges
	);

	this->updateWatchedPaths();
}

void DesktopEntryMonitor::updateWatchedPaths() {
	for (const auto& path: DesktopEntryManager::desktopPaths()) {
		addPathAndParents(this->watcher, path);
		this->scanAndWatch(path);
	}

	for (const auto& path: DesktopEntryManager::terminalConfigPaths()) {
		addPathAndParents(this->watcher, path);
	}
}

void DesktopEntryMonitor::scanAndWatch(const QString& dirPath) {
	auto dir = QDir(dirPath);
	if (!dir.exists()) return;

	this->watcher.addPath(dirPath);

	auto subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
	for (const auto& subdir: subdirs) this->watcher.addPath(subdir.absoluteFilePath());
}

void DesktopEntryMonitor::onWatchedPathChanged(const QString& /*path*/) {
	this->debounceTimer.start();
}

void DesktopEntryMonitor::processChanges() {
	this->updateWatchedPaths();
	emit this->desktopEntriesChanged();
}
