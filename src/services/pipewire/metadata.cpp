#include "metadata.hpp"
#include <array>
#include <cstring>

#include <pipewire/extensions/metadata.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/utils/json.h>

#include "registry.hpp"

namespace qs::service::pipewire {

Q_LOGGING_CATEGORY(logMeta, "quickshell.service.pipewire.metadata", QtWarningMsg);

void PwMetadata::bindHooks() {
	pw_metadata_add_listener(this->proxy(), &this->listener.hook, &PwMetadata::EVENTS, this);
}

void PwMetadata::unbindHooks() { this->listener.remove(); }

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

	emit self->registry->metadataUpdate(self, subject, key, type, value);

	// ideally we'd dealloc metadata that wasn't picked up but there's no information
	// available about if updates can come in later, so I assume they can.

	return 0; // ??? - no docs and no reason for a callback to return an int
}

PwDefaultsMetadata::PwDefaultsMetadata(PwRegistry* registry) {
	QObject::connect(
	    registry,
	    &PwRegistry::metadataUpdate,
	    this,
	    &PwDefaultsMetadata::onMetadataUpdate
	);
}

QString PwDefaultsMetadata::defaultSink() const { return this->mDefaultSink; }

QString PwDefaultsMetadata::defaultSource() const { return this->mDefaultSource; }

// we don't really care if the metadata objects are destroyed, but try to ref them so we get property updates
void PwDefaultsMetadata::onMetadataUpdate(
    PwMetadata* metadata,
    quint32 subject,
    const char* key,
    const char* /*type*/,
    const char* value
) {
	if (subject != 0) return;

	if (strcmp(key, "default.audio.sink") == 0) {
		this->defaultSinkHolder.setObject(metadata);

		auto newSink = PwDefaultsMetadata::parseNameSpaJson(value);
		qCInfo(logMeta) << "Got default sink" << newSink;
		if (newSink == this->mDefaultSink) return;

		this->mDefaultSink = newSink;
		emit this->defaultSinkChanged();
		return;
	}

	if (strcmp(key, "default.audio.source") == 0) {
		this->defaultSourceHolder.setObject(metadata);

		auto newSource = PwDefaultsMetadata::parseNameSpaJson(value);
		qCInfo(logMeta) << "Got default source" << newSource;
		if (newSource == this->mDefaultSource) return;

		this->mDefaultSource = newSource;
		emit this->defaultSourceChanged();
		return;
	}
}

QString PwDefaultsMetadata::parseNameSpaJson(const char* spaJson) {
	auto iter = std::array<spa_json, 2>();
	spa_json_init(&iter[0], spaJson, strlen(spaJson));

	if (spa_json_enter_object(&iter[0], &iter[1]) < 0) {
		qCWarning(logMeta) << "Failed to parse source/sink SPA json - failed to enter object of"
		                   << QString(spaJson);
		return "";
	}

	auto buf = std::array<char, 512>();
	while (spa_json_get_string(&iter[1], buf.data(), buf.size()) > 0) {
		if (strcmp(buf.data(), "name") != 0) continue;

		if (spa_json_get_string(&iter[1], buf.data(), buf.size()) < 0) {
			qCWarning(logMeta
			) << "Failed to parse source/sink SPA json - failed to read value of name property"
			  << QString(spaJson);
			return "";
		}

		return QString(buf.data());
	}

	qCWarning(logMeta) << "Failed to parse source/sink SPA json - failed to find name property of"
	                   << QString(spaJson);
	return "";
}

} // namespace qs::service::pipewire
