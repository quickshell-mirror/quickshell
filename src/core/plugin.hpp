#pragma once

#include <qcontainerfwd.h>
#include <qfunctionpointer.h>
#include <qlist.h>

class EngineGeneration;

class QuickshellPlugin {
public:
	QuickshellPlugin() = default;
	virtual ~QuickshellPlugin() = default;
	QuickshellPlugin(QuickshellPlugin&&) = delete;
	QuickshellPlugin(const QuickshellPlugin&) = delete;
	void operator=(QuickshellPlugin&&) = delete;
	void operator=(const QuickshellPlugin&) = delete;

	virtual QString name() { return QString(); }
	virtual QList<QString> dependencies() { return {}; }
	virtual bool applies() { return true; }
	virtual void init() {}
	virtual void registerTypes() {}
	virtual void constructGeneration(EngineGeneration& /*unused*/) {} // NOLINT
	virtual void onReload() {}

	static void registerPlugin(QuickshellPlugin& plugin);
	static void initPlugins();
	static void runConstructGeneration(EngineGeneration& generation);
	static void runOnReload();
};

// NOLINTBEGIN
#define QS_REGISTER_PLUGIN(clazz)                                                                  \
	[[gnu::constructor]] void qsInitPlugin() {                                                       \
		static clazz plugin;                                                                           \
		QuickshellPlugin::registerPlugin(plugin);                                                      \
	}
// NOLINTEND
