#include <qguiapplication.h>
#include <qqml.h>

#include "../core/plugin.hpp"
#include "panel_window.hpp"
#include "util.hpp"

namespace {

class X11Plugin: public QuickshellPlugin {
	bool applies() override { return QGuiApplication::platformName() == "xcb"; }

	void init() override { XAtom::initAtoms(); }

	void registerTypes() override {
		qmlRegisterType<XPanelInterface>("Quickshell._X11Overlay", 1, 0, "PanelWindow");

		qmlRegisterModuleImport(
		    "Quickshell",
		    QQmlModuleImportModuleAny,
		    "Quickshell._X11Overlay",
		    QQmlModuleImportLatest
		);
	}
};

QS_REGISTER_PLUGIN(X11Plugin);

} // namespace
