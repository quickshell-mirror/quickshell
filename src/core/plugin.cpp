#include "plugin.hpp"
#include <algorithm>

#include <qvector.h> // NOLINT (what??)

#include "generation.hpp"

static QVector<QsEnginePlugin*> plugins; // NOLINT

void QsEnginePlugin::registerPlugin(QsEnginePlugin& plugin) { plugins.push_back(&plugin); }

void QsEnginePlugin::initPlugins() {
	plugins.removeIf([](QsEnginePlugin* plugin) { return !plugin->applies(); });

	std::ranges::sort(plugins, [](QsEnginePlugin* a, QsEnginePlugin* b) {
		return b->dependencies().contains(a->name());
	});

	for (QsEnginePlugin* plugin: plugins) {
		plugin->init();
	}

	for (QsEnginePlugin* plugin: plugins) {
		plugin->registerTypes();
	}
}

void QsEnginePlugin::runConstructGeneration(EngineGeneration& generation) {
	for (QsEnginePlugin* plugin: plugins) {
		plugin->constructGeneration(generation);
	}
}

void QsEnginePlugin::runOnReload() {
	for (QsEnginePlugin* plugin: plugins) {
		plugin->onReload();
	}
}
