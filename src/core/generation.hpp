#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>

#include "shell.hpp"
#include "singleton.hpp"

class EngineGeneration {
public:
	explicit EngineGeneration();
	~EngineGeneration();
	Q_DISABLE_COPY_MOVE(EngineGeneration);

	// assumes root has been initialized, consumes old generation
	void onReload(EngineGeneration* old);

	static EngineGeneration* findObjectGeneration(QObject* object);

	QQmlEngine engine;
	ShellRoot* root = nullptr;
	SingletonRegistry singletonRegistry;
};
