#include "defaults.hpp"
#include <array>
#include <cstring>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <spa/utils/json.h>

#include "../../core/util.hpp"
#include "metadata.hpp"
#include "node.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

Q_LOGGING_CATEGORY(logDefaults, "quickshell.service.pipewire.defaults", QtWarningMsg);

PwDefaultTracker::PwDefaultTracker(PwRegistry* registry): registry(registry) {
	QObject::connect(registry, &PwRegistry::metadataAdded, this, &PwDefaultTracker::onMetadataAdded);
	QObject::connect(registry, &PwRegistry::nodeAdded, this, &PwDefaultTracker::onNodeAdded);
}

void PwDefaultTracker::onMetadataAdded(PwMetadata* metadata) {
	if (metadata->name() == "default") {
		qCDebug(logDefaults) << "Got new defaults metadata object" << metadata;

		if (this->defaultsMetadata.object()) {
			QObject::disconnect(this->defaultsMetadata.object(), nullptr, this, nullptr);
		}

		QObject::connect(
		    metadata,
		    &PwMetadata::propertyChanged,
		    this,
		    &PwDefaultTracker::onMetadataProperty
		);

		this->defaultsMetadata.setObject(metadata);
	}
}

void PwDefaultTracker::onMetadataProperty(const char* key, const char* type, const char* value) {
	void (PwDefaultTracker::*nodeSetter)(PwNode*) = nullptr;
	void (PwDefaultTracker::*nameSetter)(const QString&) = nullptr;

	qCDebug(logDefaults).nospace() << "Got default metadata update for " << key << ": "
	                               << QString(value);

	if (strcmp(key, "default.audio.sink") == 0) {
		nodeSetter = &PwDefaultTracker::setDefaultSink;
		nameSetter = &PwDefaultTracker::setDefaultSinkName;
	} else if (strcmp(key, "default.audio.source") == 0) {
		nodeSetter = &PwDefaultTracker::setDefaultSource;
		nameSetter = &PwDefaultTracker::setDefaultSourceName;
	} else if (strcmp(key, "default.configured.audio.sink") == 0) {
		nodeSetter = &PwDefaultTracker::setDefaultConfiguredSink;
		nameSetter = &PwDefaultTracker::setDefaultConfiguredSinkName;
	} else if (strcmp(key, "default.configured.audio.source") == 0) {
		nodeSetter = &PwDefaultTracker::setDefaultConfiguredSource;
		nameSetter = &PwDefaultTracker::setDefaultConfiguredSourceName;
	} else return;

	QString name;
	if (strcmp(type, "Spa:String:JSON") == 0) {
		auto failed = true;
		auto iter = std::array<spa_json, 2>();
		spa_json_init(&iter[0], value, strlen(value));

		if (spa_json_enter_object(&iter[0], &iter[1]) > 0) {
			auto buf = std::array<char, 1024>();

			if (spa_json_get_string(&iter[1], buf.data(), buf.size()) > 0) {
				if (strcmp(buf.data(), "name") == 0) {
					if (spa_json_get_string(&iter[1], buf.data(), buf.size()) > 0) {
						name = buf.data();
						failed = false;
					}
				}
			}
		}

		if (failed) {
			qCWarning(logDefaults) << "Failed to parse SPA default json:"
			                       << QString::fromLocal8Bit(value);
		}
	}

	(this->*nameSetter)(name);
	(this->*nodeSetter)(this->registry->findNodeByName(name));
}

void PwDefaultTracker::onNodeAdded(PwNode* node) {
	if (node->name.isEmpty()) return;

	if (this->mDefaultSink == nullptr && node->name == this->mDefaultSinkName) {
		this->setDefaultSink(node);
	}

	if (this->mDefaultSource == nullptr && node->name == this->mDefaultSourceName) {
		this->setDefaultSource(node);
	}

	if (this->mDefaultConfiguredSink == nullptr && node->name == this->mDefaultConfiguredSinkName) {
		this->setDefaultConfiguredSink(node);
	}

	if (this->mDefaultConfiguredSource == nullptr && node->name == this->mDefaultConfiguredSourceName)
	{
		this->setDefaultConfiguredSource(node);
	}
}

void PwDefaultTracker::onNodeDestroyed(QObject* node) {
	if (node == this->mDefaultSink) {
		qCInfo(logDefaults) << "Default sink destroyed.";
		this->mDefaultSink = nullptr;
		emit this->defaultSinkChanged();
	}

	if (node == this->mDefaultSource) {
		qCInfo(logDefaults) << "Default source destroyed.";
		this->mDefaultSource = nullptr;
		emit this->defaultSourceChanged();
	}

	if (node == this->mDefaultConfiguredSink) {
		qCInfo(logDefaults) << "Default configured sink destroyed.";
		this->mDefaultConfiguredSink = nullptr;
		emit this->defaultConfiguredSinkChanged();
	}

	if (node == this->mDefaultConfiguredSource) {
		qCInfo(logDefaults) << "Default configured source destroyed.";
		this->mDefaultConfiguredSource = nullptr;
		emit this->defaultConfiguredSourceChanged();
	}
}

void PwDefaultTracker::setDefaultSink(PwNode* node) {
	if (node == this->mDefaultSink) return;
	qCInfo(logDefaults) << "Default sink changed to" << node;

	setSimpleObjectHandle<
	    &PwDefaultTracker::mDefaultSink,
	    &PwDefaultTracker::onNodeDestroyed,
	    &PwDefaultTracker::defaultSinkChanged>(this, node);
}

void PwDefaultTracker::setDefaultSinkName(const QString& name) {
	if (name == this->mDefaultSinkName) return;
	qCInfo(logDefaults) << "Default sink name changed to" << name;
	this->mDefaultSinkName = name;
	emit this->defaultSinkNameChanged();
}

void PwDefaultTracker::setDefaultSource(PwNode* node) {
	if (node == this->mDefaultSource) return;
	qCInfo(logDefaults) << "Default source changed to" << node;

	setSimpleObjectHandle<
	    &PwDefaultTracker::mDefaultSource,
	    &PwDefaultTracker::onNodeDestroyed,
	    &PwDefaultTracker::defaultSourceChanged>(this, node);
}

void PwDefaultTracker::setDefaultSourceName(const QString& name) {
	if (name == this->mDefaultSourceName) return;
	qCInfo(logDefaults) << "Default source name changed to" << name;
	this->mDefaultSourceName = name;
	emit this->defaultSourceNameChanged();
}

void PwDefaultTracker::setDefaultConfiguredSink(PwNode* node) {
	if (node == this->mDefaultConfiguredSink) return;
	qCInfo(logDefaults) << "Default configured sink changed to" << node;

	setSimpleObjectHandle<
	    &PwDefaultTracker::mDefaultConfiguredSink,
	    &PwDefaultTracker::onNodeDestroyed,
	    &PwDefaultTracker::defaultConfiguredSinkChanged>(this, node);
}

void PwDefaultTracker::setDefaultConfiguredSinkName(const QString& name) {
	if (name == this->mDefaultConfiguredSinkName) return;
	qCInfo(logDefaults) << "Default configured sink name changed to" << name;
	this->mDefaultConfiguredSinkName = name;
	emit this->defaultConfiguredSinkNameChanged();
}

void PwDefaultTracker::setDefaultConfiguredSource(PwNode* node) {
	if (node == this->mDefaultConfiguredSource) return;
	qCInfo(logDefaults) << "Default configured source changed to" << node;

	setSimpleObjectHandle<
	    &PwDefaultTracker::mDefaultConfiguredSource,
	    &PwDefaultTracker::onNodeDestroyed,
	    &PwDefaultTracker::defaultConfiguredSourceChanged>(this, node);
}

void PwDefaultTracker::setDefaultConfiguredSourceName(const QString& name) {
	if (name == this->mDefaultConfiguredSourceName) return;
	qCInfo(logDefaults) << "Default configured source name changed to" << name;
	this->mDefaultConfiguredSourceName = name;
	emit this->defaultConfiguredSourceNameChanged();
}

PwNode* PwDefaultTracker::defaultSink() const { return this->mDefaultSink; }
PwNode* PwDefaultTracker::defaultSource() const { return this->mDefaultSource; }

PwNode* PwDefaultTracker::defaultConfiguredSink() const { return this->mDefaultConfiguredSink; }

const QString& PwDefaultTracker::defaultConfiguredSinkName() const {
	return this->mDefaultConfiguredSinkName;
}

PwNode* PwDefaultTracker::defaultConfiguredSource() const { return this->mDefaultConfiguredSource; }

const QString& PwDefaultTracker::defaultConfiguredSourceName() const {
	return this->mDefaultConfiguredSourceName;
}

} // namespace qs::service::pipewire
