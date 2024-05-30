#pragma once

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qpair.h>
#include <qqmlengine.h>
#include <qqmlincubator.h>
#include <qtclasshelpermacros.h>

#include "incubator.hpp"
#include "qsintercept.hpp"
#include "scan.hpp"
#include "shell.hpp"
#include "singleton.hpp"

class RootWrapper;
class QuickshellGlobal;

class EngineGeneration: public QObject {
	Q_OBJECT;

public:
	explicit EngineGeneration(const QDir& rootPath, QmlScanner scanner);
	~EngineGeneration() override;
	Q_DISABLE_COPY_MOVE(EngineGeneration);

	// assumes root has been initialized, consumes old generation
	void onReload(EngineGeneration* old);
	void setWatchingFiles(bool watching);

	void registerIncubationController(QQmlIncubationController* controller);
	void deregisterIncubationController(QQmlIncubationController* controller);

	static EngineGeneration* findEngineGeneration(QQmlEngine* engine);
	static EngineGeneration* findObjectGeneration(QObject* object);

	RootWrapper* wrapper = nullptr;
	QDir rootPath;
	QmlScanner scanner;
	QsUrlInterceptor urlInterceptor;
	QsInterceptNetworkAccessManagerFactory interceptNetFactory;
	QQmlEngine* engine = nullptr;
	ShellRoot* root = nullptr;
	SingletonRegistry singletonRegistry;
	QFileSystemWatcher* watcher = nullptr;
	QVector<QString> deletedWatchedFiles;
	DelayedQmlIncubationController delayedIncubationController;
	bool reloadComplete = false;
	QuickshellGlobal* qsgInstance = nullptr;

	void destroy();

signals:
	void filesChanged();
	void reloadFinished();

private slots:
	void onFileChanged(const QString& name);
	void onDirectoryChanged();
	void incubationControllerDestroyed();

private:
	void postReload();
	void assignIncubationController();
	QVector<QPair<QQmlIncubationController*, QObject*>> incubationControllers;
};
