#include "plugin.hpp"
#include <algorithm>

#include <qvector.h> // NOLINT (what??)

#include "generation.hpp"

static QVector<QuickshellPlugin*> plugins; // NOLINT

void QuickshellPlugin::registerPlugin(QuickshellPlugin& plugin) { plugins.push_back(&plugin); }

void QuickshellPlugin::initPlugins() {
	plugins.erase(
	    std::remove_if(
	        plugins.begin(),
	        plugins.end(),
	        [](QuickshellPlugin* plugin) { return !plugin->applies(); }
	    ),
	    plugins.end()
	);

	std::sort(plugins.begin(), plugins.end(), [](QuickshellPlugin* a, QuickshellPlugin* b) {
		return b->dependencies().contains(a->name());
	});

	for (QuickshellPlugin* plugin: plugins) {
		plugin->init();
	}

	for (QuickshellPlugin* plugin: plugins) {
		plugin->registerTypes();
	}
}

void QuickshellPlugin::runConstructGeneration(EngineGeneration& generation) {
	for (QuickshellPlugin* plugin: plugins) {
		plugin->constructGeneration(generation);
	}
}

void QuickshellPlugin::runOnReload() {
	for (QuickshellPlugin* plugin: plugins) {
		plugin->onReload();
	}
}
