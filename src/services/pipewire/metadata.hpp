#pragma once

#include <pipewire/extensions/metadata.h>
#include <pipewire/type.h>
#include <qobject.h>
#include <qstringview.h>
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
	void initProps(const spa_dict* props) override;

	[[nodiscard]] const QString& name() const;
	[[nodiscard]] bool hasSetPermission() const;

	// null value clears
	void setProperty(const char* key, const char* type, const char* value);

signals:
	void propertyChanged(const char* key, const char* type, const char* value);

private:
	static const pw_metadata_events EVENTS;
	static int
	onProperty(void* data, quint32 subject, const char* key, const char* type, const char* value);

	QString mName;

	SpaHook listener;
};

} // namespace qs::service::pipewire
