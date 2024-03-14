#pragma once

#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qtclasshelpermacros.h>

#include "qsintercept.hpp"
#include "scan.hpp"
#include "shell.hpp"
#include "singleton.hpp"

class EngineGeneration: public QObject {
	Q_OBJECT;

public:
	explicit EngineGeneration(QmlScanner scanner);
	~EngineGeneration() override;
	Q_DISABLE_COPY_MOVE(EngineGeneration);

	// assumes root has been initialized, consumes old generation
	void onReload(EngineGeneration* old);
	void setWatchingFiles(bool watching);

	static EngineGeneration* findObjectGeneration(QObject* object);

	QmlScanner scanner;
	QsInterceptNetworkAccessManagerFactory interceptNetFactory;
	QQmlEngine engine;
	ShellRoot* root = nullptr;
	SingletonRegistry singletonRegistry;
	QFileSystemWatcher* watcher = nullptr;

signals:
	void filesChanged();
};
