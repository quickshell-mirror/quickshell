#pragma once

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qlist.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlerror.h>
#include <qqmlincubator.h>
#include <qtclasshelpermacros.h>

#include "incubator.hpp"
#include "qsintercept.hpp"
#include "scan.hpp"
#include "singleton.hpp"

class RootWrapper;
class QuickshellGlobal;

class EngineGenerationExt {
public:
	EngineGenerationExt() = default;
	virtual ~EngineGenerationExt() = default;
	Q_DISABLE_COPY_MOVE(EngineGenerationExt);
};

class EngineGeneration: public QObject {
	Q_OBJECT;

public:
	explicit EngineGeneration();
	explicit EngineGeneration(const QDir& rootPath, QmlScanner scanner);
	~EngineGeneration() override;
	Q_DISABLE_COPY_MOVE(EngineGeneration);

	// assumes root has been initialized, consumes old generation
	void onReload(EngineGeneration* old);
	void setWatchingFiles(bool watching);
	bool setExtraWatchedFiles(const QVector<QString>& files);

	void registerIncubationController(QQmlIncubationController* controller);
	void deregisterIncubationController(QQmlIncubationController* controller);

	// takes ownership
	void registerExtension(const void* key, EngineGenerationExt* extension);
	EngineGenerationExt* findExtension(const void* key);

	static EngineGeneration* findEngineGeneration(const QQmlEngine* engine);
	static EngineGeneration* findObjectGeneration(const QObject* object);

	// Returns the current generation if there is only one generation,
	// otherwise null.
	static EngineGeneration* currentGeneration();

	RootWrapper* wrapper = nullptr;
	QDir rootPath;
	QmlScanner scanner;
	QsUrlInterceptor urlInterceptor;
	QsInterceptNetworkAccessManagerFactory interceptNetFactory;
	QQmlEngine* engine = nullptr;
	QObject* root = nullptr;
	SingletonRegistry singletonRegistry;
	QFileSystemWatcher* watcher = nullptr;
	QVector<QString> deletedWatchedFiles;
	QVector<QString> extraWatchedFiles;
	DelayedQmlIncubationController delayedIncubationController;
	bool reloadComplete = false;
	QuickshellGlobal* qsgInstance = nullptr;

	void destroy();
	void shutdown();

signals:
	void filesChanged();
	void reloadFinished();
	void firePostReload();

public slots:
	void quit();
	void exit(int code);

private slots:
	void onFileChanged(const QString& name);
	void onDirectoryChanged();
	void incubationControllerDestroyed();
	static void onEngineWarnings(const QList<QQmlError>& warnings);

private:
	void postReload();
	void assignIncubationController();
	QVector<QObject*> incubationControllers;
	bool incubationControllersLocked = false;
	QHash<const void*, EngineGenerationExt*> extensions;

	bool destroying = false;
	bool shouldTerminate = false;
	int exitCode = 0;
};
