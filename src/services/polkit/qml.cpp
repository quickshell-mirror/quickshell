#include "qml.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>

#include "../../core/logcat.hpp"
#include "agentimpl.hpp"

namespace {
QS_LOGGING_CATEGORY(logPolkit, "quickshell.service.polkit", QtWarningMsg);
}

namespace qs::service::polkit {
PolkitAgent::~PolkitAgent() { PolkitAgentImpl::onEndOfQmlAgent(this); };

void PolkitAgent::componentComplete() {
	if (this->mPath.isEmpty()) this->mPath = "/org/quickshell/PolkitAgent";

	auto* impl = PolkitAgentImpl::tryTakeoverOrCreate(this);
	if (impl == nullptr) return;

	this->bFlow.setBinding([impl]() { return impl->activeFlow().value(); });
	this->bIsActive.setBinding([impl]() { return impl->activeFlow().value() != nullptr; });
	this->bIsRegistered.setBinding([impl]() { return impl->isRegistered().value(); });
}

void PolkitAgent::setPath(const QString& path) {
	if (this->mPath.isEmpty()) {
		this->mPath = path;
	} else if (this->mPath != path) {
		qCWarning(logPolkit) << "cannot change path after it has been set.";
	}
}
} // namespace qs::service::polkit
