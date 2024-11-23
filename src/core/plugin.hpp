#pragma once

#include <qcontainerfwd.h>
#include <qfunctionpointer.h>
#include <qlist.h>

class EngineGeneration;

class QsEnginePlugin {
public:
	QsEnginePlugin() = default;
	virtual ~QsEnginePlugin() = default;
	QsEnginePlugin(QsEnginePlugin&&) = delete;
	QsEnginePlugin(const QsEnginePlugin&) = delete;
	void operator=(QsEnginePlugin&&) = delete;
	void operator=(const QsEnginePlugin&) = delete;

	virtual QString name() { return QString(); }
	virtual QList<QString> dependencies() { return {}; }
	virtual bool applies() { return true; }
	virtual void init() {}
	virtual void registerTypes() {}
	virtual void constructGeneration(EngineGeneration& /*unused*/) {} // NOLINT
	virtual void onReload() {}

	static void registerPlugin(QsEnginePlugin& plugin);
	static void initPlugins();
	static void runConstructGeneration(EngineGeneration& generation);
	static void runOnReload();
};

// NOLINTBEGIN
#define QS_REGISTER_PLUGIN(clazz)                                                                  \
	[[gnu::constructor]] void qsInitPlugin() {                                                       \
		static clazz plugin;                                                                           \
		QsEnginePlugin::registerPlugin(plugin);                                                        \
	}
// NOLINTEND
