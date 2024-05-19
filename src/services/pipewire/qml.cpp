#include "qml.hpp"

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "connection.hpp"
#include "link.hpp"
#include "metadata.hpp"
#include "node.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

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

	// clang-format off
	QObject::connect(&connection->defaults, &PwDefaultsMetadata::defaultSinkChanged, this, &Pipewire::defaultAudioSinkChanged);
	QObject::connect(&connection->defaults, &PwDefaultsMetadata::defaultSourceChanged, this, &Pipewire::defaultAudioSourceChanged);
	// clang-format on
}

QQmlListProperty<PwNodeIface> Pipewire::nodes() {
	return QQmlListProperty<PwNodeIface>(this, nullptr, &Pipewire::nodesCount, &Pipewire::nodeAt);
}

qsizetype Pipewire::nodesCount(QQmlListProperty<PwNodeIface>* property) {
	return static_cast<Pipewire*>(property->object)->mNodes.count(); // NOLINT
}

PwNodeIface* Pipewire::nodeAt(QQmlListProperty<PwNodeIface>* property, qsizetype index) {
	return static_cast<Pipewire*>(property->object)->mNodes.at(index); // NOLINT
}

void Pipewire::onNodeAdded(PwNode* node) {
	auto* iface = PwNodeIface::instance(node);
	QObject::connect(iface, &QObject::destroyed, this, &Pipewire::onNodeRemoved);

	this->mNodes.push_back(iface);
	emit this->nodesChanged();
}

void Pipewire::onNodeRemoved(QObject* object) {
	auto* iface = static_cast<PwNodeIface*>(object); // NOLINT
	this->mNodes.removeOne(iface);
	emit this->nodesChanged();
}

QQmlListProperty<PwLinkIface> Pipewire::links() {
	return QQmlListProperty<PwLinkIface>(this, nullptr, &Pipewire::linksCount, &Pipewire::linkAt);
}

qsizetype Pipewire::linksCount(QQmlListProperty<PwLinkIface>* property) {
	return static_cast<Pipewire*>(property->object)->mLinks.count(); // NOLINT
}

PwLinkIface* Pipewire::linkAt(QQmlListProperty<PwLinkIface>* property, qsizetype index) {
	return static_cast<Pipewire*>(property->object)->mLinks.at(index); // NOLINT
}

void Pipewire::onLinkAdded(PwLink* link) {
	auto* iface = PwLinkIface::instance(link);
	QObject::connect(iface, &QObject::destroyed, this, &Pipewire::onLinkRemoved);

	this->mLinks.push_back(iface);
	emit this->linksChanged();
}

void Pipewire::onLinkRemoved(QObject* object) {
	auto* iface = static_cast<PwLinkIface*>(object); // NOLINT
	this->mLinks.removeOne(iface);
	emit this->linksChanged();
}

QQmlListProperty<PwLinkGroupIface> Pipewire::linkGroups() {
	return QQmlListProperty<PwLinkGroupIface>(
	    this,
	    nullptr,
	    &Pipewire::linkGroupsCount,
	    &Pipewire::linkGroupAt
	);
}

qsizetype Pipewire::linkGroupsCount(QQmlListProperty<PwLinkGroupIface>* property) {
	return static_cast<Pipewire*>(property->object)->mLinkGroups.count(); // NOLINT
}

PwLinkGroupIface*
Pipewire::linkGroupAt(QQmlListProperty<PwLinkGroupIface>* property, qsizetype index) {
	return static_cast<Pipewire*>(property->object)->mLinkGroups.at(index); // NOLINT
}

void Pipewire::onLinkGroupAdded(PwLinkGroup* linkGroup) {
	auto* iface = PwLinkGroupIface::instance(linkGroup);
	QObject::connect(iface, &QObject::destroyed, this, &Pipewire::onLinkGroupRemoved);

	this->mLinkGroups.push_back(iface);
	emit this->linkGroupsChanged();
}

void Pipewire::onLinkGroupRemoved(QObject* object) {
	auto* iface = static_cast<PwLinkGroupIface*>(object); // NOLINT
	this->mLinkGroups.removeOne(iface);
	emit this->linkGroupsChanged();
}

PwNodeIface* Pipewire::defaultAudioSink() const { // NOLINT
	auto* connection = PwConnection::instance();
	auto name = connection->defaults.defaultSink();

	for (auto* node: connection->registry.nodes.values()) {
		if (name == node->name) {
			return PwNodeIface::instance(node);
		}
	}

	return nullptr;
}

PwNodeIface* Pipewire::defaultAudioSource() const { // NOLINT
	auto* connection = PwConnection::instance();
	auto name = connection->defaults.defaultSource();

	for (auto* node: connection->registry.nodes.values()) {
		if (name == node->name) {
			return PwNodeIface::instance(node);
		}
	}

	return nullptr;
}

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

	if (auto* audioBoundData = dynamic_cast<PwNodeBoundAudio*>(node->boundData)) {
		this->audioIface = new PwNodeAudioIface(audioBoundData, this);
	}
}

PwNode* PwNodeIface::node() const { return this->mNode; }

QString PwNodeIface::name() const { return this->mNode->name; }

quint32 PwNodeIface::id() const { return this->mNode->id; }

QString PwNodeIface::description() const { return this->mNode->description; }

QString PwNodeIface::nickname() const { return this->mNode->nick; }

bool PwNodeIface::isSink() const { return this->mNode->isSink; }

bool PwNodeIface::isStream() const { return this->mNode->isStream; }

QVariantMap PwNodeIface::properties() const {
	auto map = QVariantMap();
	for (auto [k, v]: this->mNode->properties.asKeyValueRange()) {
		map.insert(k, QVariant::fromValue(v));
	}

	return map;
}

PwNodeAudioIface* PwNodeIface::audio() const { return this->audioIface; }

PwNodeIface* PwNodeIface::instance(PwNode* node) {
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
	QObject::connect(group, &QObject::destroyed, this, [this]() { delete this; });
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
		if (auto* pwObject = dynamic_cast<PwObjectRefIface*>(object)) {
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
