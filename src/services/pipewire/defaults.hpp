#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

#include "registry.hpp"

namespace qs::service::pipewire {

class PwDefaultTracker: public QObject {
	Q_OBJECT;

public:
	explicit PwDefaultTracker(PwRegistry* registry);
	void reset();

	[[nodiscard]] PwNode* defaultSink() const;
	[[nodiscard]] PwNode* defaultSource() const;

	[[nodiscard]] PwNode* defaultConfiguredSink() const;
	[[nodiscard]] const QString& defaultConfiguredSinkName() const;
	void changeConfiguredSink(PwNode* node);
	void changeConfiguredSinkName(const QString& sink);

	[[nodiscard]] PwNode* defaultConfiguredSource() const;
	[[nodiscard]] const QString& defaultConfiguredSourceName() const;
	void changeConfiguredSource(PwNode* node);
	void changeConfiguredSourceName(const QString& source);

signals:
	void defaultSinkChanged();
	void defaultSinkNameChanged();

	void defaultSourceChanged();
	void defaultSourceNameChanged();

	void defaultConfiguredSinkChanged();
	void defaultConfiguredSinkNameChanged();

	void defaultConfiguredSourceChanged();
	void defaultConfiguredSourceNameChanged();

private slots:
	void onMetadataAdded(PwMetadata* metadata);
	void onMetadataProperty(const char* key, const char* type, const char* value);
	void onNodeAdded(PwNode* node);
	void onDefaultSinkDestroyed();
	void onDefaultSourceDestroyed();
	void onDefaultConfiguredSinkDestroyed();
	void onDefaultConfiguredSourceDestroyed();

private:
	void setDefaultSink(PwNode* node);
	void setDefaultSinkName(const QString& name);

	void setDefaultSource(PwNode* node);
	void setDefaultSourceName(const QString& name);

	void setDefaultConfiguredSink(PwNode* node);
	void setDefaultConfiguredSinkName(const QString& name);

	void setDefaultConfiguredSource(PwNode* node);
	void setDefaultConfiguredSourceName(const QString& name);

	bool setConfiguredDefault(const char* key, const QString& value);

	PwRegistry* registry;
	PwBindableRef<PwMetadata> defaultsMetadata;

	PwNode* mDefaultSink = nullptr;
	QString mDefaultSinkName;

	PwNode* mDefaultSource = nullptr;
	QString mDefaultSourceName;

	PwNode* mDefaultConfiguredSink = nullptr;
	QString mDefaultConfiguredSinkName;

	PwNode* mDefaultConfiguredSource = nullptr;
	QString mDefaultConfiguredSourceName;
};

} // namespace qs::service::pipewire
