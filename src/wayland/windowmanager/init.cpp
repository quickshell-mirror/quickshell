#include <qguiapplication.h>

#include "../../core/plugin.hpp"

namespace qs::wm::wayland {
void installWmProvider();
}

namespace {

class WaylandWmPlugin: public QsEnginePlugin {
	QList<QString> dependencies() override { return {"window"}; }

	bool applies() override { return QGuiApplication::platformName() == "wayland"; }

	void init() override { qs::wm::wayland::installWmProvider(); }
};

QS_REGISTER_PLUGIN(WaylandWmPlugin);

} // namespace
