#include <qguiapplication.h>
#include <qqml.h>

#include "../core/plugin.hpp"
#include "waylandlayershell.hpp"

namespace {

class WaylandPlugin: public QuickshellPlugin {
	bool applies() override { return QGuiApplication::platformName() == "wayland"; }

	void registerTypes() override {
		qmlRegisterType<WaylandPanelInterface>("QuickShell._WaylandOverlay", 1, 0, "PanelWindow");

		// If any types are defined inside a module using QML_ELEMENT then all QML_ELEMENT types
		// will not be registered. This can be worked around with a module import which makes
		// the QML_ELMENT module import the old register-type style module.
		qmlRegisterModuleImport(
		    "QuickShell",
		    QQmlModuleImportModuleAny,
		    "QuickShell._WaylandOverlay",
		    QQmlModuleImportLatest
		);
	}
};

QS_REGISTER_PLUGIN(WaylandPlugin);

} // namespace
