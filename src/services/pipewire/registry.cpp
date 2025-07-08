#include "registry.hpp"
#include <cstring>

#include <pipewire/core.h>
#include <pipewire/device.h>
#include <pipewire/extensions/metadata.h>
#include <pipewire/link.h>
#include <pipewire/node.h>
#include <pipewire/proxy.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "core.hpp"
#include "device.hpp"
#include "link.hpp"
#include "metadata.hpp"
#include "node.hpp"

namespace qs::service::pipewire {

QS_LOGGING_CATEGORY(logRegistry, "quickshell.service.pipewire.registry", QtWarningMsg);

PwBindableObject::~PwBindableObject() {
	if (this->id != 0) {
		qCFatal(logRegistry) << "Destroyed pipewire object" << this
		                     << "without causing safeDestroy. THIS IS UNDEFINED BEHAVIOR.";
	}
}

void PwBindableObject::init(PwRegistry* registry, quint32 id, quint32 perms) {
	this->id = id;
	this->perms = perms;
	this->registry = registry;
	this->setParent(registry);
	qCDebug(logRegistry) << "Creating object" << this;
}

void PwBindableObject::safeDestroy() {
	this->unbind();
	qCDebug(logRegistry) << "Destroying object" << this;
	emit this->destroying(this);
	this->id = 0;
	delete this;
}

void PwBindableObject::debugId(QDebug& debug) const {
	auto saver = QDebugStateSaver(debug);
	debug.nospace() << this->id << "/" << (this->object == nullptr ? "unbound" : "bound");
}

void PwBindableObject::ref() {
	this->refcount++;
	if (this->refcount == 1) this->bind();
}

void PwBindableObject::unref() {
	this->refcount--;
	if (this->refcount == 0) this->unbind();
}

void PwBindableObject::registryBind(const char* interface, quint32 version) {
	// NOLINTNEXTLINE
	auto* object = pw_registry_bind(this->registry->object, this->id, interface, version, 0);
	this->object = static_cast<pw_proxy*>(object);
}

void PwBindableObject::bind() {
	qCDebug(logRegistry) << "Bound object" << this;
	this->bindHooks();
}

void PwBindableObject::unbind() {
	if (this->object == nullptr) return;
	qCDebug(logRegistry) << "Unbinding object" << this;
	this->unbindHooks();
	pw_proxy_destroy(this->object);
	this->object = nullptr;
}

QDebug operator<<(QDebug debug, const PwBindableObject* object) {
	if (object == nullptr) {
		debug << "PwBindableObject(0x0)";
	} else {
		auto saver = QDebugStateSaver(debug);
		// 0 if not present, start of class name if present
		auto idx = QString(object->metaObject()->className()).lastIndexOf(':') + 1;
		debug.nospace() << (object->metaObject()->className() + idx) << '(' // NOLINT
		                << static_cast<const void*>(object) << ", id=";
		object->debugId(debug);
		debug << ')';
	}

	return debug;
}

PwBindableObjectRef::PwBindableObjectRef(PwBindableObject* object) { this->setObject(object); }

PwBindableObjectRef::~PwBindableObjectRef() { this->setObject(nullptr); }

void PwBindableObjectRef::setObject(PwBindableObject* object) {
	if (this->mObject != nullptr) {
		this->mObject->unref();
		QObject::disconnect(this->mObject, nullptr, this, nullptr);
	}

	this->mObject = object;

	if (object != nullptr) {
		this->mObject->ref();
		QObject::connect(object, &QObject::destroyed, this, &PwBindableObjectRef::onObjectDestroyed);
	}
}

void PwBindableObjectRef::onObjectDestroyed() {
	// allow references to it so consumers can disconnect themselves
	emit this->objectDestroyed();
	this->mObject = nullptr;
}

void PwRegistry::init(PwCore& core) {
	this->core = &core;
	this->object = pw_core_get_registry(core.core, PW_VERSION_REGISTRY, 0);
	pw_registry_add_listener(this->object, &this->listener.hook, &PwRegistry::EVENTS, this);

	QObject::connect(this->core, &PwCore::synced, this, &PwRegistry::onCoreSync);

	qCDebug(logRegistry) << "Registry created. Sending core sync for initial object tracking.";
	this->coreSyncSeq = this->core->sync(PW_ID_CORE);
}

void PwRegistry::onCoreSync(quint32 id, qint32 seq) {
	if (id != PW_ID_CORE || seq != this->coreSyncSeq) return;

	switch (this->initState) {
	case InitState::SendingObjects:
		qCDebug(logRegistry) << "Initial sync for objects received. Syncing for metadata binding.";
		this->coreSyncSeq = this->core->sync(PW_ID_CORE);
		this->initState = InitState::Binding;
		break;
	case InitState::Binding:
		qCInfo(logRegistry) << "Initial state sync complete.";
		this->initState = InitState::Done;
		emit this->initialized();
		break;
	default: break;
	}
}

const pw_registry_events PwRegistry::EVENTS = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = &PwRegistry::onGlobal,
    .global_remove = &PwRegistry::onGlobalRemoved,
};

void PwRegistry::onGlobal(
    void* data,
    quint32 id,
    quint32 permissions,
    const char* type,
    quint32 /*version*/,
    const spa_dict* props
) {
	auto* self = static_cast<PwRegistry*>(data);

	if (strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0) {
		auto* meta = new PwMetadata();
		meta->init(self, id, permissions);
		meta->initProps(props);

		self->metadata.emplace(id, meta);
		emit self->metadataAdded(meta);
	} else if (strcmp(type, PW_TYPE_INTERFACE_Link) == 0) {
		auto* link = new PwLink();
		link->init(self, id, permissions);
		link->initProps(props);

		self->links.emplace(id, link);
		self->addLinkToGroup(link);
		emit self->linkAdded(link);
	} else if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
		auto* node = new PwNode();
		node->init(self, id, permissions);
		node->initProps(props);

		self->nodes.emplace(id, node);
		emit self->nodeAdded(node);
	} else if (strcmp(type, PW_TYPE_INTERFACE_Device) == 0) {
		auto* device = new PwDevice();
		device->init(self, id, permissions);
		device->initProps(props);

		self->devices.emplace(id, device);
	}
}

void PwRegistry::onGlobalRemoved(void* data, quint32 id) {
	auto* self = static_cast<PwRegistry*>(data);

	if (auto* meta = self->metadata.value(id)) {
		self->metadata.remove(id);
		meta->safeDestroy();
	} else if (auto* link = self->links.value(id)) {
		self->links.remove(id);
		link->safeDestroy();
	} else if (auto* node = self->nodes.value(id)) {
		self->nodes.remove(id);
		node->safeDestroy();
	}
}

void PwRegistry::addLinkToGroup(PwLink* link) {
	for (auto* group: this->linkGroups) {
		if (group->tryAddLink(link)) return;
	}

	auto* group = new PwLinkGroup(link);
	QObject::connect(group, &QObject::destroyed, this, &PwRegistry::onLinkGroupDestroyed);
	this->linkGroups.push_back(group);
	emit this->linkGroupAdded(group);
}

void PwRegistry::onLinkGroupDestroyed(QObject* object) {
	auto* group = static_cast<PwLinkGroup*>(object); // NOLINT
	this->linkGroups.removeOne(group);
}

PwNode* PwRegistry::findNodeByName(QStringView name) const {
	if (name.isEmpty()) return nullptr;

	for (auto* node: this->nodes.values()) {
		if (node->name == name) return node;
	}

	return nullptr;
}

} // namespace qs::service::pipewire
