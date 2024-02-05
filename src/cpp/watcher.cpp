#include "watcher.hpp"

#include <qdir.h>
#include <qfileinfo.h>
#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qtmetamacros.h>

FiletreeWatcher::FiletreeWatcher(QObject* parent): QObject(parent) {
	QObject::connect(
	    &this->watcher,
	    &QFileSystemWatcher::fileChanged,
	    this,
	    &FiletreeWatcher::onFileChanged
	);

	QObject::connect(
	    &this->watcher,
	    &QFileSystemWatcher::directoryChanged,
	    this,
	    &FiletreeWatcher::onDirectoryChanged
	);
}
void FiletreeWatcher::addPath(const QString& path) {
	this->watcher.addPath(path);

	if (QFileInfo(path).isDir()) {
		auto dir = QDir(path);

		for (auto& entry: dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
			this->addPath(dir.filePath(entry));
		}
	}
}

void FiletreeWatcher::onDirectoryChanged(const QString& path) { this->addPath(path); }

void FiletreeWatcher::onFileChanged(const QString& path) { emit this->fileChanged(path); }
