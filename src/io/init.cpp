#include "../core/plugin.hpp"
#include "process.hpp"

namespace {

class IoPlugin: public QsEnginePlugin {
	void onReload() override { DisownedProcessContext::destroyInstance(); }
};

QS_REGISTER_PLUGIN(IoPlugin);

} // namespace
