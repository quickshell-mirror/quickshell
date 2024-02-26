#include "plugin.hpp"
#include <algorithm>

#include <qvector.h> // NOLINT (what??)

// defined by moc. see below comment.
void qml_register_types_QuickShell(); // NOLINT

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

	for (QuickshellPlugin* plugin: plugins) {
		plugin->init();
	}

	// This seems incredibly stupid but it appears qt will not register the module types
	// if types are already defined for the module, meaning qmlRegisterType does not work
	// unless the module types are already registered.
	qml_register_types_QuickShell();

	for (QuickshellPlugin* plugin: plugins) {
		plugin->registerTypes();
	}
}
