#include <qguiapplication.h>
#include <qlist.h>
#include <qlogging.h>
#include <qqml.h>
#include <qtenvironmentvariables.h>

#include "../core/plugin.hpp"

#ifdef QS_WAYLAND_WLR_LAYERSHELL
#include "wlr_layershell/wlr_layershell.hpp"
#endif

void installPlatformMenuHook(); // NOLINT(misc-use-internal-linkage)
void installPopupPositioner();  // NOLINT(misc-use-internal-linkage)

namespace {

class WaylandPlugin: public QsEnginePlugin {
	QList<QString> dependencies() override { return {"window"}; }

	bool applies() override {
		auto isWayland = QGuiApplication::platformName() == "wayland";

		if (!isWayland && !qEnvironmentVariable("WAYLAND_DISPLAY").isEmpty()) {
			qWarning() << "--- WARNING ---";
			qWarning() << "WAYLAND_DISPLAY is present but QT_QPA_PLATFORM is"
			           << QGuiApplication::platformName();
			qWarning() << "If you are actually running wayland, set QT_QPA_PLATFORM to \"wayland\" or "
			              "most functionality will be broken.";
		}

		return isWayland;
	}

	void init() override {
		installPlatformMenuHook();
		installPopupPositioner();
	}

	void registerTypes() override {
#ifdef QS_WAYLAND_WLR_LAYERSHELL
		qmlRegisterType<qs::wayland::layershell::WaylandPanelInterface>(
		    "Quickshell._WaylandOverlay",
		    1,
		    0,
		    "PanelWindow"
		);

		// If any types are defined inside a module using QML_ELEMENT then all QML_ELEMENT types
		// will not be registered. This can be worked around with a module import which makes
		// the QML_ELMENT module import the old register-type style module.

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
