#pragma once

#include <qcontainerfwd.h>
#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qpair.h>
#include <qqmlincubator.h>
#include <qtclasshelpermacros.h>

#include "incubator.hpp"
#include "qsintercept.hpp"
#include "scan.hpp"
#include "shell.hpp"
#include "singleton.hpp"

class RootWrapper;

class EngineGeneration: public QObject {
	Q_OBJECT;

public:
	explicit EngineGeneration(QmlScanner scanner);
	~EngineGeneration() override;
	Q_DISABLE_COPY_MOVE(EngineGeneration);

	// assumes root has been initialized, consumes old generation
	void onReload(EngineGeneration* old);
	void setWatchingFiles(bool watching);

	void registerIncubationController(QQmlIncubationController* controller);
	void deregisterIncubationController(QQmlIncubationController* controller);

	static EngineGeneration* findObjectGeneration(QObject* object);

	RootWrapper* wrapper = nullptr;
	QmlScanner scanner;
	QsUrlInterceptor urlInterceptor;
	QsInterceptNetworkAccessManagerFactory interceptNetFactory;
	QQmlEngine* engine = nullptr;
	ShellRoot* root = nullptr;
	SingletonRegistry singletonRegistry;
	QFileSystemWatcher* watcher = nullptr;
	DelayedQmlIncubationController delayedIncubationController;
	bool reloadComplete = false;

	void destroy();

signals:
	void filesChanged();
	void reloadFinished();

private slots:
	void incubationControllerDestroyed();

private:
	void postReload();
	void assignIncubationController();
	QVector<QPair<QQmlIncubationController*, QObject*>> incubationControllers;
};
