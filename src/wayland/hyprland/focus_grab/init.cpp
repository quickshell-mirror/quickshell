#include <qqml.h>

#include "../../../core/plugin.hpp"

namespace {

class HyprlandFocusGrabPlugin: public QuickshellPlugin {
	void registerTypes() override {
		qmlRegisterModuleImport(
		    "Quickshell.Hyprland",
		    QQmlModuleImportModuleAny,
		    "Quickshell.Hyprland._FocusGrab",
		    QQmlModuleImportLatest
		);
	}
};

QS_REGISTER_PLUGIN(HyprlandFocusGrabPlugin);

} // namespace
