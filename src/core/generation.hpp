#pragma once

#include <qcontainerfwd.h>
#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qqmlincubator.h>
#include <qtclasshelpermacros.h>

#include "incubator.hpp"
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

	void registerIncubationController(QQmlIncubationController* controller);

	static EngineGeneration* findObjectGeneration(QObject* object);

	QmlScanner scanner;
	QsInterceptNetworkAccessManagerFactory interceptNetFactory;
	QQmlEngine engine;
	ShellRoot* root = nullptr;
	SingletonRegistry singletonRegistry;
	QFileSystemWatcher* watcher = nullptr;
	DelayedQmlIncubationController delayedIncubationController;

signals:
	void filesChanged();

private slots:
	void incubationControllerDestroyed();

private:
	void assignIncubationController();
	QVector<QQmlIncubationController*> incubationControllers;
};
