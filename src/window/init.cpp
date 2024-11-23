#include <qqml.h>
#include <qstring.h>

#include "../core/plugin.hpp"

namespace {

class WindowPlugin: public QsEnginePlugin {
	// _Window has to be registered before wayland or x11 modules, otherwise module overlays
	// will apply in the wrong order.
	QString name() override { return "window"; }

	void registerTypes() override {
		qmlRegisterModuleImport(
		    "Quickshell",
		    QQmlModuleImportModuleAny,
		    "Quickshell._Window",
		    QQmlModuleImportLatest
		);
	}
};

QS_REGISTER_PLUGIN(WindowPlugin);

} // namespace
