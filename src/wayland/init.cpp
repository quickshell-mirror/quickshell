#include <qguiapplication.h>
#include <qqml.h>

#include "../core/plugin.hpp"

#ifdef QS_WAYLAND_WLR_LAYERSHELL
#include "wlr_layershell.hpp"
#endif

namespace {

class WaylandPlugin: public QuickshellPlugin {
	bool applies() override { return QGuiApplication::platformName() == "wayland"; }

	void registerTypes() override {
#ifdef QS_WAYLAND_WLR_LAYERSHELL
		qmlRegisterType<WaylandPanelInterface>("Quickshell._WaylandOverlay", 1, 0, "PanelWindow");

		// If any types are defined inside a module using QML_ELEMENT then all QML_ELEMENT types
		// will not be registered. This can be worked around with a module import which makes
		// the QML_ELMENT module import the old register-type style module.

		qmlRegisterModuleImport(
		    "Quickshell.Wayland",
		    QQmlModuleImportModuleAny,
		    "Quickshell.Wayland._WlrLayerShell",
		    QQmlModuleImportLatest
		);

		qmlRegisterModuleImport(
		    "Quickshell",
		    QQmlModuleImportModuleAny,
		    "Quickshell._WaylandOverlay",
		    QQmlModuleImportLatest
		);
#endif
	}
};

QS_REGISTER_PLUGIN(WaylandPlugin);

} // namespace
