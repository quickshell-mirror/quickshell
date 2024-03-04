#include "../core/plugin.hpp"
#include "process.hpp"

namespace {

class IoPlugin: public QuickshellPlugin {
	void onReload() override { DisownedProcessContext::destroyInstance(); }
};

QS_REGISTER_PLUGIN(IoPlugin);

} // namespace
