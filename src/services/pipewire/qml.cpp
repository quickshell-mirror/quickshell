#include "qml.hpp"

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../core/model.hpp"
#include "connection.hpp"
#include "defaults.hpp"
#include "link.hpp"
#include "node.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

PwObjectIface::PwObjectIface(PwBindableObject* object): QObject(object), object(object) {
	// We want to destroy the interface before QObject::destroyed is fired, as handlers
	// connected before PwObjectIface will run first and emit signals that hit user code,
	// which can then try to reference the iface again after ~PwNode() has been called but
	// before ~QObject() has finished.
	QObject::connect(object, &PwBindableObject::destroying, this, &PwObjectIface::onObjectDestroying);
}

void PwObjectIface::onObjectDestroying() { delete this; }

void PwObjectIface::ref() {
	this->refcount++;

	if (this->refcount == 1) {
		this->object->ref();
	}
}

void PwObjectIface::unref() {
	if (this->refcount == 0) return;
	this->refcount--;

	if (this->refcount == 0) {
		this->object->unref();
	}
}

Pipewire::Pipewire(QObject* parent): QObject(parent) {
	auto* connection = PwConnection::instance();

	for (auto* node: connection->registry.nodes.values()) {
		this->onNodeAdded(node);
	}

	QObject::connect(&connection->registry, &PwRegistry::nodeAdded, this, &Pipewire::onNodeAdded);

	for (auto* link: connection->registry.links.values()) {
		this->onLinkAdded(link);
	}

	QObject::connect(&connection->registry, &PwRegistry::linkAdded, this, &Pipewire::onLinkAdded);

	for (auto* group: connection->registry.linkGroups) {
		this->onLinkGroupAdded(group);
	}

	QObject::connect(
	    &connection->registry,
	    &PwRegistry::linkGroupAdded,
	    this,
	    &Pipewire::onLinkGroupAdded
	);

	QObject::connect(
	    &connection->defaults,
	    &PwDefaultTracker::defaultSinkChanged,
	    this,
	    &Pipewire::defaultAudioSinkChanged
	);

	QObject::connect(
	    &connection->defaults,
	    &PwDefaultTracker::defaultSourceChanged,
	    this,
	    &Pipewire::defaultAudioSourceChanged
	);

	QObject::connect(
	    &connection->defaults,
	    &PwDefaultTracker::defaultConfiguredSinkChanged,
	    this,
	    &Pipewire::defaultConfiguredAudioSinkChanged
	);

	QObject::connect(
	    &connection->defaults,
	    &PwDefaultTracker::defaultConfiguredSourceChanged,
	    this,
	    &Pipewire::defaultConfiguredAudioSourceChanged
	);

	if (!connection->registry.isInitialized()) {
		QObject::connect(
		    &connection->registry,
		    &PwRegistry::initialized,
		    this,
		    &Pipewire::readyChanged,
		    Qt::SingleShotConnection
		);
	}
}

ObjectModel<PwNodeIface>* Pipewire::nodes() { return &this->mNodes; }

void Pipewire::onNodeAdded(PwNode* node) {
	auto* iface = PwNodeIface::instance(node);
	QObject::connect(iface, &QObject::destroyed, this, &Pipewire::onNodeRemoved);
	this->mNodes.insertObject(iface);
}

void Pipewire::onNodeRemoved(QObject* object) {
	auto* iface = static_cast<PwNodeIface*>(object); // NOLINT
	this->mNodes.removeObject(iface);
}

ObjectModel<PwLinkIface>* Pipewire::links() { return &this->mLinks; }

void Pipewire::onLinkAdded(PwLink* link) {
	auto* iface = PwLinkIface::instance(link);
	QObject::connect(iface, &QObject::destroyed, this, &Pipewire::onLinkRemoved);
	this->mLinks.insertObject(iface);
}

void Pipewire::onLinkRemoved(QObject* object) {
	auto* iface = static_cast<PwLinkIface*>(object); // NOLINT
	this->mLinks.removeObject(iface);
}

ObjectModel<PwLinkGroupIface>* Pipewire::linkGroups() { return &this->mLinkGroups; }

void Pipewire::onLinkGroupAdded(PwLinkGroup* linkGroup) {
	auto* iface = PwLinkGroupIface::instance(linkGroup);
	QObject::connect(iface, &QObject::destroyed, this, &Pipewire::onLinkGroupRemoved);
	this->mLinkGroups.insertObject(iface);
}

void Pipewire::onLinkGroupRemoved(QObject* object) {
	auto* iface = static_cast<PwLinkGroupIface*>(object); // NOLINT
	this->mLinkGroups.removeObject(iface);
}

PwNodeIface* Pipewire::defaultAudioSink() const { // NOLINT
	auto* node = PwConnection::instance()->defaults.defaultSink();
	return PwNodeIface::instance(node);
}

PwNodeIface* Pipewire::defaultAudioSource() const { // NOLINT
	auto* node = PwConnection::instance()->defaults.defaultSource();
	return PwNodeIface::instance(node);
}

PwNodeIface* Pipewire::defaultConfiguredAudioSink() const { // NOLINT
	auto* node = PwConnection::instance()->defaults.defaultConfiguredSink();
	return PwNodeIface::instance(node);
}

void Pipewire::setDefaultConfiguredAudioSink(PwNodeIface* node) {
	PwConnection::instance()->defaults.changeConfiguredSink(node ? node->node() : nullptr);
}

PwNodeIface* Pipewire::defaultConfiguredAudioSource() const { // NOLINT
	auto* node = PwConnection::instance()->defaults.defaultConfiguredSource();
	return PwNodeIface::instance(node);
}

void Pipewire::setDefaultConfiguredAudioSource(PwNodeIface* node) {
	PwConnection::instance()->defaults.changeConfiguredSource(node ? node->node() : nullptr);
}

bool Pipewire::isReady() { return PwConnection::instance()->registry.isInitialized(); }

PwNodeIface* PwNodeLinkTracker::node() const { return this->mNode; }

void PwNodeLinkTracker::setNode(PwNodeIface* node) {
	if (node == this->mNode) return;

	if (this->mNode != nullptr) {
		if (node == nullptr) {
			QObject::disconnect(&PwConnection::instance()->registry, nullptr, this, nullptr);
		}

		QObject::disconnect(this->mNode, nullptr, this, nullptr);
	}

	if (node != nullptr) {
		if (this->mNode == nullptr) {
			QObject::connect(
			    &PwConnection::instance()->registry,
			    &PwRegistry::linkGroupAdded,
			    this,
			    &PwNodeLinkTracker::onLinkGroupCreated
			);
		}

		QObject::connect(node, &QObject::destroyed, this, &PwNodeLinkTracker::onNodeDestroyed);
	}

	this->mNode = node;
	this->updateLinks();
	emit this->nodeChanged();
}

void PwNodeLinkTracker::updateLinks() {
	// done first to avoid unref->reref of nodes
	auto newLinks = QVector<PwLinkGroupIface*>();
	if (this->mNode != nullptr) {
		auto* connection = PwConnection::instance();

		for (auto* link: connection->registry.linkGroups) {
			if ((!this->mNode->isSink() && link->outputNode() == this->mNode->id())
			    || (this->mNode->isSink() && link->inputNode() == this->mNode->id()))
			{
				auto* iface = PwLinkGroupIface::instance(link);

				// do not connect twice
				if (!this->mLinkGroups.contains(iface)) {
					QObject::connect(
					    iface,
					    &QObject::destroyed,
					    this,
					    &PwNodeLinkTracker::onLinkGroupDestroyed
					);
				}

				newLinks.push_back(iface);
			}
		}
	}

	for (auto* iface: this->mLinkGroups) {
		// only disconnect no longer used nodes
		if (!newLinks.contains(iface)) {
			QObject::disconnect(iface, nullptr, this, nullptr);
		}
	}

	this->mLinkGroups = newLinks;
	emit this->linkGroupsChanged();
}

QQmlListProperty<PwLinkGroupIface> PwNodeLinkTracker::linkGroups() {
	return QQmlListProperty<PwLinkGroupIface>(
	    this,
	    nullptr,
	    &PwNodeLinkTracker::linkGroupsCount,
	    &PwNodeLinkTracker::linkGroupAt
	);
}

qsizetype PwNodeLinkTracker::linkGroupsCount(QQmlListProperty<PwLinkGroupIface>* property) {
	return static_cast<PwNodeLinkTracker*>(property->object)->mLinkGroups.count(); // NOLINT
}

PwLinkGroupIface*
PwNodeLinkTracker::linkGroupAt(QQmlListProperty<PwLinkGroupIface>* property, qsizetype index) {
	return static_cast<PwNodeLinkTracker*>(property->object)->mLinkGroups.at(index); // NOLINT
}

void PwNodeLinkTracker::onNodeDestroyed() {
	this->mNode = nullptr;
	QObject::disconnect(&PwConnection::instance()->registry, nullptr, this, nullptr);

	this->updateLinks();
	emit this->nodeChanged();
}

void PwNodeLinkTracker::onLinkGroupCreated(PwLinkGroup* linkGroup) {
	if ((!this->mNode->isSink() && linkGroup->outputNode() == this->mNode->id())
	    || (this->mNode->isSink() && linkGroup->inputNode() == this->mNode->id()))
	{
		auto* iface = PwLinkGroupIface::instance(linkGroup);
		QObject::connect(iface, &QObject::destroyed, this, &PwNodeLinkTracker::onLinkGroupDestroyed);
		this->mLinkGroups.push_back(iface);
		emit this->linkGroupsChanged();
	}
}

void PwNodeLinkTracker::onLinkGroupDestroyed(QObject* object) {
	if (this->mLinkGroups.removeOne(object)) {
		emit this->linkGroupsChanged();
	}
}

PwNodeAudioIface::PwNodeAudioIface(PwNodeBoundAudio* boundData, QObject* parent)
    : QObject(parent)
    , boundData(boundData) {
	// clang-format off
	QObject::connect(boundData, &PwNodeBoundAudio::mutedChanged, this, &PwNodeAudioIface::mutedChanged);
	QObject::connect(boundData, &PwNodeBoundAudio::channelsChanged, this, &PwNodeAudioIface::channelsChanged);
	QObject::connect(boundData, &PwNodeBoundAudio::volumesChanged, this, &PwNodeAudioIface::volumesChanged);
	// clang-format on
}

bool PwNodeAudioIface::isMuted() const { return this->boundData->isMuted(); }

void PwNodeAudioIface::setMuted(bool muted) { this->boundData->setMuted(muted); }

float PwNodeAudioIface::averageVolume() const { return this->boundData->averageVolume(); }

void PwNodeAudioIface::setAverageVolume(float volume) { this->boundData->setAverageVolume(volume); }

QVector<PwAudioChannel::Enum> PwNodeAudioIface::channels() const {
	return this->boundData->channels();
}

QVector<float> PwNodeAudioIface::volumes() const { return this->boundData->volumes(); }

void PwNodeAudioIface::setVolumes(const QVector<float>& volumes) {
	this->boundData->setVolumes(volumes);
}

PwNodeIface::PwNodeIface(PwNode* node): PwObjectIface(node), mNode(node) {
	QObject::connect(node, &PwNode::propertiesChanged, this, &PwNodeIface::propertiesChanged);
	QObject::connect(node, &PwNode::readyChanged, this, &PwNodeIface::readyChanged);

	if (auto* audioBoundData = dynamic_cast<PwNodeBoundAudio*>(node->boundData)) {
		this->audioIface = new PwNodeAudioIface(audioBoundData, this);
	}
}

PwNode* PwNodeIface::node() const { return this->mNode; }

QString PwNodeIface::name() const { return this->mNode->name; }

quint32 PwNodeIface::id() const { return this->mNode->id; }

QString PwNodeIface::description() const { return this->mNode->description; }

QString PwNodeIface::nickname() const { return this->mNode->nick; }

bool PwNodeIface::isSink() const { return this->mNode->type.testFlags(PwNodeType::Sink); }

bool PwNodeIface::isStream() const { return this->mNode->type.testFlags(PwNodeType::Stream); }

bool PwNodeIface::isReady() const { return this->mNode->ready; }

PwNodeType::Flags PwNodeIface::type() const { return this->mNode->type; };

QVariantMap PwNodeIface::properties() const {
	auto map = QVariantMap();
	for (auto [k, v]: this->mNode->properties.asKeyValueRange()) {
		map.insert(k, QVariant::fromValue(v));
	}

	return map;
}

PwNodeAudioIface* PwNodeIface::audio() const { return this->audioIface; }

PwNodeIface* PwNodeIface::instance(PwNode* node) {
	if (node == nullptr) return nullptr;

	auto v = node->property("iface");
	if (v.canConvert<PwNodeIface*>()) {
		return v.value<PwNodeIface*>();
	}

	auto* instance = new PwNodeIface(node);
	node->setProperty("iface", QVariant::fromValue(instance));

	return instance;
}

PwLinkIface::PwLinkIface(PwLink* link): PwObjectIface(link), mLink(link) {
	QObject::connect(link, &PwLink::stateChanged, this, &PwLinkIface::stateChanged);
}

PwLink* PwLinkIface::link() const { return this->mLink; }

quint32 PwLinkIface::id() const { return this->mLink->id; }

PwNodeIface* PwLinkIface::target() const {
	return PwNodeIface::instance(
	    PwConnection::instance()->registry.nodes.value(this->mLink->inputNode())
	);
}

PwNodeIface* PwLinkIface::source() const {
	return PwNodeIface::instance(
	    PwConnection::instance()->registry.nodes.value(this->mLink->outputNode())
	);
}

PwLinkState::Enum PwLinkIface::state() const { return this->mLink->state(); }

PwLinkIface* PwLinkIface::instance(PwLink* link) {
	auto v = link->property("iface");
	if (v.canConvert<PwLinkIface*>()) {
		return v.value<PwLinkIface*>();
	}

	auto* instance = new PwLinkIface(link);
	link->setProperty("iface", QVariant::fromValue(instance));

	return instance;
}

PwLinkGroupIface::PwLinkGroupIface(PwLinkGroup* group): QObject(group), mGroup(group) {
	QObject::connect(group, &PwLinkGroup::stateChanged, this, &PwLinkGroupIface::stateChanged);
}

void PwLinkGroupIface::ref() { this->mGroup->ref(); }

void PwLinkGroupIface::unref() { this->mGroup->unref(); }

PwLinkGroup* PwLinkGroupIface::group() const { return this->mGroup; }

PwNodeIface* PwLinkGroupIface::target() const {
	return PwNodeIface::instance(
	    PwConnection::instance()->registry.nodes.value(this->mGroup->inputNode())
	);
}

PwNodeIface* PwLinkGroupIface::source() const {
	return PwNodeIface::instance(
	    PwConnection::instance()->registry.nodes.value(this->mGroup->outputNode())
	);
}

PwLinkState::Enum PwLinkGroupIface::state() const { return this->mGroup->state(); }

PwLinkGroupIface* PwLinkGroupIface::instance(PwLinkGroup* group) {
	auto v = group->property("iface");
	if (v.canConvert<PwLinkGroupIface*>()) {
		return v.value<PwLinkGroupIface*>();
	}

	auto* instance = new PwLinkGroupIface(group);
	group->setProperty("iface", QVariant::fromValue(instance));

	return instance;
}

PwObjectTracker::~PwObjectTracker() { this->clearList(); }

QList<QObject*> PwObjectTracker::objects() const { return this->trackedObjects; }

void PwObjectTracker::setObjects(const QList<QObject*>& objects) {
	// +1 ref before removing old refs to avoid an unbind->bind.
	for (auto* object: objects) {
		if (auto* pwObject = dynamic_cast<PwObjectRefIface*>(object)) {
			pwObject->ref();
		}
	}

	this->clearList();

	// connect destroy
	for (auto* object: objects) {
		if (dynamic_cast<PwObjectRefIface*>(object) != nullptr) {
			QObject::connect(object, &QObject::destroyed, this, &PwObjectTracker::objectDestroyed);
		}
	}

	this->trackedObjects = objects;
}

void PwObjectTracker::clearList() {
	for (auto* object: this->trackedObjects) {
		if (auto* pwObject = dynamic_cast<PwObjectRefIface*>(object)) {
			pwObject->unref();
			QObject::disconnect(object, nullptr, this, nullptr);
		}
	}

	this->trackedObjects.clear();
}

void PwObjectTracker::objectDestroyed(QObject* object) {
	this->trackedObjects.removeOne(object);
	emit this->objectsChanged();
}

} // namespace qs::service::pipewire
