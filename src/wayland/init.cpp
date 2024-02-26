#include <qguiapplication.h>
#include <qqml.h>

#include "../core/plugin.hpp"
#include "waylandlayershell.hpp"

namespace {

class WaylandPlugin: public QuickshellPlugin {
	bool applies() override { return QGuiApplication::platformName() == "wayland"; }

	void registerTypes() override {
		qmlRegisterType<WaylandPanelInterface>("QuickShell", 1, 0, "PanelWindow");
	}
};

QS_REGISTER_PLUGIN(WaylandPlugin);

} // namespace
