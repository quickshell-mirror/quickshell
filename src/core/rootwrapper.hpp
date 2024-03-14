#pragma once

#include <qobject.h>
#include <qqmlengine.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qurl.h>

#include "generation.hpp"
#include "watcher.hpp"

class RootWrapper: public QObject {
	Q_OBJECT;

public:
	explicit RootWrapper(QString rootPath);
	~RootWrapper() override;
	Q_DISABLE_COPY_MOVE(RootWrapper);

	void reloadGraph(bool hard);

private slots:
	void onWatchFilesChanged();
	void onWatchedFilesChanged();

private:
	QString rootPath;
	EngineGeneration* generation = nullptr;
	FiletreeWatcher* configWatcher = nullptr;
	QString originalWorkingDirectory;
};
