#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/model.hpp"
#include "link.hpp"
#include "node.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

class PwNodeIface;
class PwLinkIface;
class PwLinkGroupIface;

class PwObjectRefIface {
public:
	PwObjectRefIface() = default;
	virtual ~PwObjectRefIface() = default;
	Q_DISABLE_COPY_MOVE(PwObjectRefIface);

	virtual void ref() = 0;
	virtual void unref() = 0;
};

class PwObjectIface
    : public QObject
    , public PwObjectRefIface {
	Q_OBJECT;

public:
	explicit PwObjectIface(PwBindableObject* object): QObject(object), object(object) {};
	// destructor should ONLY be called by the pw object destructor, making an unref unnecessary
	~PwObjectIface() override = default;
	Q_DISABLE_COPY_MOVE(PwObjectIface);

	void ref() override;
	void unref() override;

private:
	quint32 refcount = 0;
	PwBindableObject* object;
};

///! Contains links to all pipewire objects.
class Pipewire: public QObject {
	Q_OBJECT;
	// clang-format off
	/// All pipewire nodes.
	Q_PROPERTY(ObjectModel<PwNodeIface>* nodes READ nodes CONSTANT);
	/// All pipewire links.
	Q_PROPERTY(ObjectModel<PwLinkIface>* links READ links CONSTANT);
	/// All pipewire link groups.
	Q_PROPERTY(ObjectModel<PwLinkGroupIface>* linkGroups READ linkGroups CONSTANT);
	/// The default audio sink or `null`.
	Q_PROPERTY(PwNodeIface* defaultAudioSink READ defaultAudioSink NOTIFY defaultAudioSinkChanged);
	/// The default audio source or `null`.
	Q_PROPERTY(PwNodeIface* defaultAudioSource READ defaultAudioSource NOTIFY defaultAudioSourceChanged);
	// clang-format on
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit Pipewire(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<PwNodeIface>* nodes();
	[[nodiscard]] ObjectModel<PwLinkIface>* links();
	[[nodiscard]] ObjectModel<PwLinkGroupIface>* linkGroups();
	[[nodiscard]] PwNodeIface* defaultAudioSink() const;
	[[nodiscard]] PwNodeIface* defaultAudioSource() const;

signals:
	void defaultAudioSinkChanged();
	void defaultAudioSourceChanged();

private slots:
	void onNodeAdded(PwNode* node);
	void onNodeRemoved(QObject* object);
	void onLinkAdded(PwLink* link);
	void onLinkRemoved(QObject* object);
	void onLinkGroupAdded(PwLinkGroup* group);
	void onLinkGroupRemoved(QObject* object);

private:
	ObjectModel<PwNodeIface> mNodes {this};
	ObjectModel<PwLinkIface> mLinks {this};
	ObjectModel<PwLinkGroupIface> mLinkGroups {this};
};

///! Tracks all link connections to a given node.
class PwNodeLinkTracker: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The node to track connections to.
	Q_PROPERTY(PwNodeIface* node READ node WRITE setNode NOTIFY nodeChanged);
	/// Link groups connected to the given node.
	///
	/// If the node is a sink, links which target the node will be tracked.
	/// If the node is a source, links which source the node will be tracked.
	Q_PROPERTY(QQmlListProperty<PwLinkGroupIface> linkGroups READ linkGroups NOTIFY linkGroupsChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit PwNodeLinkTracker(QObject* parent = nullptr): QObject(parent) {}

	[[nodiscard]] PwNodeIface* node() const;
	void setNode(PwNodeIface* node);

	[[nodiscard]] QQmlListProperty<PwLinkGroupIface> linkGroups();

signals:
	void nodeChanged();
	void linkGroupsChanged();

private slots:
	void onNodeDestroyed();
	void onLinkGroupCreated(PwLinkGroup* linkGroup);
	void onLinkGroupDestroyed(QObject* object);

private:
	static qsizetype linkGroupsCount(QQmlListProperty<PwLinkGroupIface>* property);
	static PwLinkGroupIface*
	linkGroupAt(QQmlListProperty<PwLinkGroupIface>* property, qsizetype index);

	void updateLinks();

	PwNodeIface* mNode = nullptr;
	QVector<PwLinkGroupIface*> mLinkGroups;
};

///! Audio specific properties of pipewire nodes.
class PwNodeAudioIface: public QObject {
	Q_OBJECT;
	/// If the node is currently muted. Setting this property changes the mute state.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged);
	/// The average volume over all channels of the node.
	/// Setting this property modifies the volume of all channels proportionately.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(float volume READ averageVolume WRITE setAverageVolume NOTIFY volumesChanged);
	/// The audio channels present on the node.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(QVector<PwAudioChannel::Enum> channels READ channels NOTIFY channelsChanged);
	/// The volumes of each audio channel individually. Each entry corrosponds to
	/// the channel at the same index in `channels`. `volumes` and `channels` will always be
	/// the same length.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(QVector<float> volumes READ volumes WRITE setVolumes NOTIFY volumesChanged);
	QML_NAMED_ELEMENT(PwNodeAudio);
	QML_UNCREATABLE("PwNodeAudio cannot be created directly");

public:
	explicit PwNodeAudioIface(PwNodeBoundAudio* boundData, QObject* parent);

	[[nodiscard]] bool isMuted() const;
	void setMuted(bool muted);

	[[nodiscard]] float averageVolume() const;
	void setAverageVolume(float volume);

	[[nodiscard]] QVector<PwAudioChannel::Enum> channels() const;

	[[nodiscard]] QVector<float> volumes() const;
	void setVolumes(const QVector<float>& volumes);

signals:
	void mutedChanged();
	void channelsChanged();
	void volumesChanged();

private:
	PwNodeBoundAudio* boundData;
};

///! A node in the pipewire connection graph.
class PwNodeIface: public PwObjectIface {
	Q_OBJECT;
	/// The pipewire object id of the node.
	///
	/// Mainly useful for debugging. you can inspect the node directly
	/// with `pw-cli i <id>`.
	Q_PROPERTY(quint32 id READ id CONSTANT);
	/// The node's name, corrosponding to the object's `node.name` property.
	Q_PROPERTY(QString name READ name CONSTANT);
	/// The node's description, corrosponding to the object's `node.description` property.
	///
	/// May be empty. Generally more human readable than `name`.
	Q_PROPERTY(QString description READ description CONSTANT);
	/// The node's nickname, corrosponding to the object's `node.nickname` property.
	///
	/// May be empty. Generally but not always more human readable than `description`.
	Q_PROPERTY(QString nickname READ nickname CONSTANT);
	/// If `true`, then the node accepts audio input from other nodes,
	/// if `false` the node outputs audio to other nodes.
	Q_PROPERTY(bool isSink READ isSink CONSTANT);
	/// If `true` then the node is likely to be a program, if false it is liekly to be hardware.
	Q_PROPERTY(bool isStream READ isStream CONSTANT);
	/// The property set present on the node, as an object containing key-value pairs.
	/// You can inspect this directly with `pw-cli i <id>`.
	///
	/// A few properties of note, which may or may not be present:
	/// - `application.name` - A suggested human readable name for the node.
	/// - `application.icon-name` - The name of an icon recommended to display for the node.
	/// - `media.name` - A description of the currently playing media.
	///   (more likely to be present than `media.title` and `media.artist`)
	/// - `media.title` - The title of the currently playing media.
	/// - `media.artist` - The artist of the currently playing media.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(QVariantMap properties READ properties NOTIFY propertiesChanged);
	/// Extra information present only if the node sends or receives audio.
	Q_PROPERTY(PwNodeAudioIface* audio READ audio CONSTANT);
	QML_NAMED_ELEMENT(PwNode);
	QML_UNCREATABLE("PwNodes cannot be created directly");

public:
	explicit PwNodeIface(PwNode* node);

	[[nodiscard]] PwNode* node() const;
	[[nodiscard]] quint32 id() const;
	[[nodiscard]] QString name() const;
	[[nodiscard]] QString description() const;
	[[nodiscard]] QString nickname() const;
	[[nodiscard]] bool isSink() const;
	[[nodiscard]] bool isStream() const;
	[[nodiscard]] QVariantMap properties() const;
	[[nodiscard]] PwNodeAudioIface* audio() const;

	static PwNodeIface* instance(PwNode* node);

signals:
	void propertiesChanged();

private:
	PwNode* mNode;
	PwNodeAudioIface* audioIface = nullptr;
};

///! A connection between pipewire nodes.
/// Note that there is one link per *channel* of a connection between nodes.
/// You usually want [PwLinkGroup](../pwlinkgroup).
class PwLinkIface: public PwObjectIface {
	Q_OBJECT;
	/// The pipewire object id of the link.
	///
	/// Mainly useful for debugging. you can inspect the link directly
	/// with `pw-cli i <id>`.
	Q_PROPERTY(quint32 id READ id CONSTANT);
	/// The node that is *receiving* information. (the sink)
	Q_PROPERTY(PwNodeIface* target READ target CONSTANT);
	/// The node that is *sending* information. (the source)
	Q_PROPERTY(PwNodeIface* source READ source CONSTANT);
	/// The current state of the link.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(PwLinkState::Enum state READ state NOTIFY stateChanged);
	QML_NAMED_ELEMENT(PwLink);
	QML_UNCREATABLE("PwLinks cannot be created directly");

public:
	explicit PwLinkIface(PwLink* link);

	[[nodiscard]] PwLink* link() const;
	[[nodiscard]] quint32 id() const;
	[[nodiscard]] PwNodeIface* target() const;
	[[nodiscard]] PwNodeIface* source() const;
	[[nodiscard]] PwLinkState::Enum state() const;

	static PwLinkIface* instance(PwLink* link);

signals:
	void stateChanged();

private:
	PwLink* mLink;
};

///! A group of connections between pipewire nodes.
/// A group of connections between pipewire nodes, one per source->target pair.
class PwLinkGroupIface
    : public QObject
    , public PwObjectRefIface {
	Q_OBJECT;
	/// The node that is *receiving* information. (the sink)
	Q_PROPERTY(PwNodeIface* target READ target CONSTANT);
	/// The node that is *sending* information. (the source)
	Q_PROPERTY(PwNodeIface* source READ source CONSTANT);
	/// The current state of the link group.
	///
	/// > [!WARNING] This property is invalid unless the node is [bound](../pwobjecttracker).
	Q_PROPERTY(PwLinkState::Enum state READ state NOTIFY stateChanged);
	QML_NAMED_ELEMENT(PwLinkGroup);
	QML_UNCREATABLE("PwLinkGroups cannot be created directly");

public:
	explicit PwLinkGroupIface(PwLinkGroup* group);
	// destructor should ONLY be called by the pw object destructor, making an unref unnecessary
	~PwLinkGroupIface() override = default;
	Q_DISABLE_COPY_MOVE(PwLinkGroupIface);

	void ref() override;
	void unref() override;

	[[nodiscard]] PwLinkGroup* group() const;
	[[nodiscard]] PwNodeIface* target() const;
	[[nodiscard]] PwNodeIface* source() const;
	[[nodiscard]] PwLinkState::Enum state() const;

	static PwLinkGroupIface* instance(PwLinkGroup* group);

signals:
	void stateChanged();

private:
	PwLinkGroup* mGroup;
};

///! Binds pipewire objects.
/// If the object list of at least one PwObjectTracker contains a given pipewire object,
/// it will become *bound* and you will be able to interact with bound-only properties.
class PwObjectTracker: public QObject {
	Q_OBJECT;
	/// The list of objects to bind.
	Q_PROPERTY(QList<QObject*> objects READ objects WRITE setObjects NOTIFY objectsChanged);
	QML_ELEMENT;

public:
	explicit PwObjectTracker(QObject* parent = nullptr): QObject(parent) {}
	~PwObjectTracker() override;
	Q_DISABLE_COPY_MOVE(PwObjectTracker);

	[[nodiscard]] QList<QObject*> objects() const;
	void setObjects(const QList<QObject*>& objects);

signals:
	void objectsChanged();

private slots:
	void objectDestroyed(QObject* object);

private:
	void clearList();

	QList<QObject*> trackedObjects;
};

} // namespace qs::service::pipewire
