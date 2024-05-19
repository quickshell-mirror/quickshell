#pragma once

#include <pipewire/extensions/metadata.h>
#include <pipewire/type.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "core.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

constexpr const char TYPE_INTERFACE_Metadata[] = PW_TYPE_INTERFACE_Metadata; // NOLINT
class PwMetadata
    : public PwBindable<pw_metadata, TYPE_INTERFACE_Metadata, PW_VERSION_METADATA> { // NOLINT
	Q_OBJECT;

public:
	void bindHooks() override;
	void unbindHooks() override;

private:
	static const pw_metadata_events EVENTS;
	static int
	onProperty(void* data, quint32 subject, const char* key, const char* type, const char* value);

	SpaHook listener;
};

class PwDefaultsMetadata: public QObject {
	Q_OBJECT;

public:
	explicit PwDefaultsMetadata(PwRegistry* registry);

	[[nodiscard]] QString defaultSource() const;
	[[nodiscard]] QString defaultSink() const;

signals:
	void defaultSourceChanged();
	void defaultSinkChanged();

private slots:
	void onMetadataUpdate(
	    PwMetadata* metadata,
	    quint32 subject,
	    const char* key,
	    const char* type,
	    const char* value
	);

private:
	static QString parseNameSpaJson(const char* spaJson);

	PwBindableRef<PwMetadata> defaultSinkHolder;
	PwBindableRef<PwMetadata> defaultSourceHolder;

	bool sinkConfigured = false;
	QString mDefaultSink;
	bool sourceConfigured = false;
	QString mDefaultSource;
};

} // namespace qs::service::pipewire
