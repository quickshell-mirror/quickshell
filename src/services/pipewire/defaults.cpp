#include "defaults.hpp"
#include <array>
#include <cstring>

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtmetamacros.h>
#include <spa/utils/json.h>

#include "../../core/logcat.hpp"
#include "metadata.hpp"
#include "node.hpp"
#include "registry.hpp"

// This and spa_json_init are part of json-core.h, which is missing from older pw versions.
struct spa_json;

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logDefaults, "quickshell.service.pipewire.defaults", QtWarningMsg);
}

PwDefaultTracker::PwDefaultTracker(PwRegistry* registry): registry(registry) {
	QObject::connect(registry, &PwRegistry::metadataAdded, this, &PwDefaultTracker::onMetadataAdded);
	QObject::connect(registry, &PwRegistry::nodeAdded, this, &PwDefaultTracker::onNodeAdded);
}

void PwDefaultTracker::reset() {
	if (auto* meta = this->defaultsMetadata.object()) {
		QObject::disconnect(meta, nullptr, this, nullptr);
	}

	this->defaultsMetadata.setObject(nullptr);
	this->setDefaultSink(nullptr);
	this->setDefaultSinkName(QString());
	this->setDefaultSource(nullptr);
	this->setDefaultSourceName(QString());
	this->setDefaultConfiguredSink(nullptr);
	this->setDefaultConfiguredSinkName(QString());
	this->setDefaultConfiguredSource(nullptr);
	this->setDefaultConfiguredSourceName(QString());
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

	if (key == nullptr) return; // NOLINT(bugprone-branch-clone)
	else if (strcmp(key, "default.audio.sink") == 0) {
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
	if (type != nullptr && value != nullptr && strcmp(type, "Spa:String:JSON") == 0) {
		auto failed = true;
		auto iter = std::array<spa_json, 2>();
		spa_json_init(&iter[0], value, strlen(value)); // NOLINT (misc-include-cleaner)

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

void PwDefaultTracker::changeConfiguredSink(PwNode* node) {
	if (node != nullptr) {
		if (!node->type.testFlags(PwNodeType::AudioSink)) {
			qCCritical(logDefaults) << "Cannot change default sink to a node that is not a sink.";
			return;
		}

		this->changeConfiguredSinkName(node->name);
	} else {
		this->changeConfiguredSinkName("");
	}
}

void PwDefaultTracker::changeConfiguredSinkName(const QString& sink) {
	if (sink == this->mDefaultConfiguredSinkName) return;

	if (this->setConfiguredDefault("default.configured.audio.sink", sink)) {
		this->mDefaultConfiguredSinkName = sink;
		qCInfo(logDefaults) << "Set default configured sink to" << sink;
	}
}

void PwDefaultTracker::changeConfiguredSource(PwNode* node) {
	if (node != nullptr) {
		if (!node->type.testFlags(PwNodeType::AudioSource)) {
			qCCritical(logDefaults) << "Cannot change default source to a node that is not a source.";
			return;
		}

		this->changeConfiguredSourceName(node->name);
	} else {
		this->changeConfiguredSourceName("");
	}
}

void PwDefaultTracker::changeConfiguredSourceName(const QString& source) {
	if (source == this->mDefaultConfiguredSourceName) return;

	if (this->setConfiguredDefault("default.configured.audio.source", source)) {
		this->mDefaultConfiguredSourceName = source;
		qCInfo(logDefaults) << "Set default configured source to" << source;
	}
}

bool PwDefaultTracker::setConfiguredDefault(const char* key, const QString& value) {
	auto* meta = this->defaultsMetadata.object();

	if (!meta || !meta->proxy()) {
		qCCritical(logDefaults) << "Cannot set default node as metadata is not ready.";
		return false;
	}

	if (!meta->hasSetPermission()) {
		qCCritical(
		    logDefaults
		) << "Cannot set default node as write+execute permissions are missing for"
		  << meta;
		return false;
	}

	if (value.isEmpty()) {
		meta->setProperty(key, "Spa:String:JSON", nullptr);
	} else {
		// Spa json is a superset of json so we can avoid the awful spa json api when serializing.
		auto json = QJsonDocument({{"name", value}}).toJson(QJsonDocument::Compact);

		meta->setProperty(key, "Spa:String:JSON", json.toStdString().c_str());
	}

	return true;
}

void PwDefaultTracker::setDefaultSink(PwNode* node) {
	if (node == this->mDefaultSink) return;
	qCInfo(logDefaults) << "Default sink changed to" << node;

	if (this->mDefaultSink != nullptr) {
		QObject::disconnect(this->mDefaultSink, nullptr, this, nullptr);
	}

	this->mDefaultSink = node;

	if (node != nullptr) {
		QObject::connect(node, &QObject::destroyed, this, &PwDefaultTracker::onDefaultSinkDestroyed);
	}

	emit this->defaultSinkChanged();
}

void PwDefaultTracker::onDefaultSinkDestroyed() {
	qCInfo(logDefaults) << "Default sink destroyed.";
	this->mDefaultSink = nullptr;
	emit this->defaultSinkChanged();
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

	if (this->mDefaultSource != nullptr) {
		QObject::disconnect(this->mDefaultSource, nullptr, this, nullptr);
	}

	this->mDefaultSource = node;

	if (node != nullptr) {
		QObject::connect(node, &QObject::destroyed, this, &PwDefaultTracker::onDefaultSourceDestroyed);
	}

	emit this->defaultSourceChanged();
}

void PwDefaultTracker::onDefaultSourceDestroyed() {
	qCInfo(logDefaults) << "Default source destroyed.";
	this->mDefaultSource = nullptr;
	emit this->defaultSourceChanged();
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

	if (this->mDefaultConfiguredSink != nullptr) {
		QObject::disconnect(this->mDefaultConfiguredSink, nullptr, this, nullptr);
	}

	this->mDefaultConfiguredSink = node;

	if (node != nullptr) {
		QObject::connect(
		    node,
		    &QObject::destroyed,
		    this,
		    &PwDefaultTracker::onDefaultConfiguredSinkDestroyed
		);
	}

	emit this->defaultConfiguredSinkChanged();
}

void PwDefaultTracker::onDefaultConfiguredSinkDestroyed() {
	qCInfo(logDefaults) << "Default configured sink destroyed.";
	this->mDefaultConfiguredSink = nullptr;
	emit this->defaultConfiguredSinkChanged();
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

	if (this->mDefaultConfiguredSource != nullptr) {
		QObject::disconnect(this->mDefaultConfiguredSource, nullptr, this, nullptr);
	}

	this->mDefaultConfiguredSource = node;

	if (node != nullptr) {
		QObject::connect(
		    node,
		    &QObject::destroyed,
		    this,
		    &PwDefaultTracker::onDefaultConfiguredSourceDestroyed
		);
	}

	emit this->defaultConfiguredSourceChanged();
}

void PwDefaultTracker::onDefaultConfiguredSourceDestroyed() {
	qCInfo(logDefaults) << "Default configured source destroyed.";
	this->mDefaultConfiguredSource = nullptr;
	emit this->defaultConfiguredSourceChanged();
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
