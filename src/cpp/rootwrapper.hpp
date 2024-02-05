#pragma once

#include <qobject.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qurl.h>

#include "scavenge.hpp"
#include "shell.hpp"
#include "watcher.hpp"

class RootWrapper: public QObject, virtual public Scavengeable {
	Q_OBJECT;

public:
	explicit RootWrapper(QString rootPath);

	QObject* scavengeTargetFor(QObject* child) override;

	void reloadGraph(bool hard);

private slots:
	void onConfigChanged();
	void onWatchedFilesChanged();

private:
	QString rootPath;
	QQmlEngine engine;
	ShellRoot* root = nullptr;
	FiletreeWatcher* configWatcher = nullptr;
};
