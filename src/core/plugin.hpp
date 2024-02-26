#pragma once

#include <qcontainerfwd.h>
#include <qfunctionpointer.h>

class QuickshellPlugin {
public:
	QuickshellPlugin() = default;
	virtual ~QuickshellPlugin() = default;
	QuickshellPlugin(QuickshellPlugin&&) = delete;
	QuickshellPlugin(const QuickshellPlugin&) = delete;
	void operator=(QuickshellPlugin&&) = delete;
	void operator=(const QuickshellPlugin&) = delete;

	virtual bool applies() { return true; }
	virtual void init() {}
	virtual void registerTypes() {}

	static void registerPlugin(QuickshellPlugin& plugin);
	static void initPlugins();
};

// NOLINTBEGIN
#define QS_REGISTER_PLUGIN(clazz)                                                                  \
	[[gnu::constructor]] void qsInitPlugin() {                                                       \
		static clazz plugin;                                                                           \
		QuickshellPlugin::registerPlugin(plugin);                                                      \
	}
// NOLINTEND
