#include "../../core/generation.hpp"
#include "../../core/plugin.hpp"
#include "trayimageprovider.hpp"

namespace {

class SniPlugin: public QuickshellPlugin {
	void constructGeneration(EngineGeneration& generation) override {
		generation.engine->addImageProvider("service.sni", new qs::service::sni::TrayImageProvider());
	}
};

QS_REGISTER_PLUGIN(SniPlugin);

} // namespace
