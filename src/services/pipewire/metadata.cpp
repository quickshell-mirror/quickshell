#include "metadata.hpp"

#include <pipewire/core.h>
#include <pipewire/extensions/metadata.h>
#include <pipewire/permission.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/utils/dict.h>

#include "../../core/logcat.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logMeta, "quickshell.service.pipewire.metadata", QtWarningMsg);
}

void PwMetadata::bindHooks() {
	pw_metadata_add_listener(this->proxy(), &this->listener.hook, &PwMetadata::EVENTS, this);
}

void PwMetadata::unbindHooks() { this->listener.remove(); }

void PwMetadata::initProps(const spa_dict* props) {
	if (const auto* name = spa_dict_lookup(props, PW_KEY_METADATA_NAME)) {
		this->mName = name;
	}
}

const QString& PwMetadata::name() const { return this->mName; }

const pw_metadata_events PwMetadata::EVENTS = {
    .version = PW_VERSION_METADATA_EVENTS,
    .property = &PwMetadata::onProperty,
};

int PwMetadata::onProperty(
    void* data,
    quint32 subject,
    const char* key,
    const char* type,
    const char* value
) {
	auto* self = static_cast<PwMetadata*>(data);
	qCDebug(logMeta) << "Received metadata for" << self << "- subject:" << subject
	                 << "key:" << QString(key) << "type:" << QString(type)
	                 << "value:" << QString(value);

	emit self->propertyChanged(key, type, value);

	return 0; // ??? - no docs and no reason for a callback to return an int
}

bool PwMetadata::hasSetPermission() const {
	return (this->perms & (PW_PERM_W | PW_PERM_X)) == (PW_PERM_W | PW_PERM_X);
}

void PwMetadata::setProperty(const char* key, const char* type, const char* value) {
	if (this->proxy() == nullptr) {
		qCCritical(logMeta) << "Tried to change property of" << this << "which is not bound.";
		return;
	}

	if (!this->hasSetPermission()) {
		qCCritical(logMeta) << "Tried to change property of" << this
		                    << "which is missing write+execute permissions.";
	}

	pw_metadata_set_property(this->proxy(), PW_ID_CORE, key, type, value);
}

} // namespace qs::service::pipewire
