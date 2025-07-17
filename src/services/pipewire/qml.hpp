#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/doc.hpp"
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
	explicit PwObjectIface(PwBindableObject* object);
	// destructor should ONLY be called by the pw object destructor, making an unref unnecessary
	~PwObjectIface() override = default;
	Q_DISABLE_COPY_MOVE(PwObjectIface);

	void ref() override;
	void unref() override;

private slots:
	void onObjectDestroying();

private:
	quint32 refcount = 0;
	PwBindableObject* object;
};

///! Contains links to all pipewire objects.
class Pipewire: public QObject {
	Q_OBJECT;
	// clang-format off
	/// All nodes present in pipewire.
	///
	/// This list contains every node on the system.
	/// To find a useful subset, filtering with the following properties may be helpful:
	/// - @@PwNode.isStream - if the node is an application or hardware device.
	/// - @@PwNode.isSink - if the node is a sink or source.
	/// - @@PwNode.audio - if non null the node is an audio node.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::pipewire::PwNodeIface>*);
	Q_PROPERTY(UntypedObjectModel* nodes READ nodes CONSTANT);
	/// All links present in pipewire.
	///
	/// Links connect pipewire nodes to each other, and can be used to determine
	/// their relationship.
	///
	/// If you already have a node you want to check for connections to,
	/// use @@PwNodeLinkTracker instead of filtering this list.
	///
	/// > [!INFO] Multiple links may exist between the same nodes. See @@linkGroups
	/// > for a deduplicated list containing only one entry per link between nodes.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::pipewire::PwLinkIface>*);
	Q_PROPERTY(UntypedObjectModel* links READ links CONSTANT);
	/// All link groups present in pipewire.
	///
	/// The same as @@links but deduplicated.
	///
	/// If you already have a node you want to check for connections to,
	/// use @@PwNodeLinkTracker instead of filtering this list.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::pipewire::PwLinkGroupIface>*);
	Q_PROPERTY(UntypedObjectModel* linkGroups READ linkGroups CONSTANT);
	/// The default audio sink (output) or `null`.
	///
	/// This is the default sink currently in use by pipewire, and the one applications
	/// are currently using.
	///
	/// To set the default sink, use @@preferredDefaultAudioSink.
	///
	/// > [!INFO] When the default sink changes, this property may breifly become null.
	/// > This depends on your hardware.
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* defaultAudioSink READ defaultAudioSink NOTIFY defaultAudioSinkChanged);
	/// The default audio source (input) or `null`.
	///
	/// This is the default source currently in use by pipewire, and the one applications
	/// are currently using.
	///
	/// To set the default source, use @@preferredDefaultAudioSource.
	///
	/// > [!INFO] When the default source changes, this property may breifly become null.
	/// > This depends on your hardware.
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* defaultAudioSource READ defaultAudioSource NOTIFY defaultAudioSourceChanged);
	/// The preferred default audio sink (output) or `null`.
	///
	/// This is a hint to pipewire telling it which sink should be the default when possible.
	/// @@defaultAudioSink may differ when it is not possible for pipewire to pick this node.
	///
	/// See @@defaultAudioSink for the current default sink, regardless of preference.
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* preferredDefaultAudioSink READ defaultConfiguredAudioSink WRITE setDefaultConfiguredAudioSink NOTIFY defaultConfiguredAudioSinkChanged);
	/// The preferred default audio source (input) or `null`.
	///
	/// This is a hint to pipewire telling it which source should be the default when possible.
	/// @@defaultAudioSource may differ when it is not possible for pipewire to pick this node.
	///
	/// See @@defaultAudioSource for the current default source, regardless of preference.
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* preferredDefaultAudioSource READ defaultConfiguredAudioSource WRITE setDefaultConfiguredAudioSource NOTIFY defaultConfiguredAudioSourceChanged);
	/// This property is true if quickshell has completed its initial sync with
	/// the pipewire server. If true, nodes, links and sync/source preferences will be
	/// in a good state.
	///
	/// > [!NOTE] You can use the pipewire object before it is ready, but some nodes/links
	/// > may be missing, and preference metadata may be null.
	Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged);
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

	[[nodiscard]] PwNodeIface* defaultConfiguredAudioSink() const;
	static void setDefaultConfiguredAudioSink(PwNodeIface* node);

	[[nodiscard]] PwNodeIface* defaultConfiguredAudioSource() const;
	static void setDefaultConfiguredAudioSource(PwNodeIface* node);

	[[nodiscard]] static bool isReady();

signals:
	void defaultAudioSinkChanged();
	void defaultAudioSourceChanged();

	void defaultConfiguredAudioSinkChanged();
	void defaultConfiguredAudioSourceChanged();

	void readyChanged();

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
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* node READ node WRITE setNode NOTIFY nodeChanged);
	/// Link groups connected to the given node.
	///
	/// If the node is a sink, links which target the node will be tracked.
	/// If the node is a source, links which source the node will be tracked.
	Q_PROPERTY(QQmlListProperty<qs::service::pipewire::PwLinkGroupIface> linkGroups READ linkGroups NOTIFY linkGroupsChanged);
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
/// Extra properties of a @@PwNode if the node is an audio node.
///
/// See @@PwNode.audio.
class PwNodeAudioIface: public QObject {
	Q_OBJECT;
	// clang-format off
	/// If the node is currently muted. Setting this property changes the mute state.
	///
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
	Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged);
	/// The average volume over all channels of the node.
	/// Setting this property modifies the volume of all channels proportionately.
	///
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
	Q_PROPERTY(float volume READ averageVolume WRITE setAverageVolume NOTIFY volumesChanged);
	/// The audio channels present on the node.
	///
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
	Q_PROPERTY(QVector<qs::service::pipewire::PwAudioChannel::Enum> channels READ channels NOTIFY channelsChanged);
	/// The volumes of each audio channel individually. Each entry corrosponds to
	/// the volume of the channel at the same index in @@channels. @@volumes and @@channels
	/// will always be the same length.
	///
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
	Q_PROPERTY(QVector<float> volumes READ volumes WRITE setVolumes NOTIFY volumesChanged);
	// clang-format on
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
	/// Mainly useful for debugging. You can inspect the node directly
	/// with `pw-cli i <id>`.
	Q_PROPERTY(quint32 id READ id CONSTANT);
	/// The node's name, corrosponding to the object's `node.name` property.
	Q_PROPERTY(QString name READ name CONSTANT);
	/// The node's description, corrosponding to the object's `node.description` property.
	///
	/// May be empty. Generally more human readable than @@name.
	Q_PROPERTY(QString description READ description CONSTANT);
	/// The node's nickname, corrosponding to the object's `node.nickname` property.
	///
	/// May be empty. Generally but not always more human readable than @@description.
	Q_PROPERTY(QString nickname READ nickname CONSTANT);
	/// If `true`, then the node accepts audio input from other nodes,
	/// if `false` the node outputs audio to other nodes.
	Q_PROPERTY(bool isSink READ isSink CONSTANT);
	/// If `true` then the node is likely to be a program, if `false` it is likely to be
	/// a hardware device.
	Q_PROPERTY(bool isStream READ isStream CONSTANT);
	/// The type of this node. Reflects Pipewire's [media.class](https://docs.pipewire.org/page_man_pipewire-props_7.html).
	Q_PROPERTY(qs::service::pipewire::PwNodeType::Flags type READ type CONSTANT);
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
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
	Q_PROPERTY(QVariantMap properties READ properties NOTIFY propertiesChanged);
	/// Extra information present only if the node sends or receives audio.
	///
	/// The presence or absence of this property can be used to determine if a node
	/// manages audio, regardless of if it is bound. If non null, the node is an audio node.
	Q_PROPERTY(qs::service::pipewire::PwNodeAudioIface* audio READ audio CONSTANT);
	/// True if the node is fully bound and ready to use.
	///
	/// > [!NOTE] The node may be used before it is fully bound, but some data
	/// > may be missing or incorrect.
	Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged);
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
	[[nodiscard]] bool isReady() const;
	[[nodiscard]] PwNodeType::Flags type() const;
	[[nodiscard]] QVariantMap properties() const;
	[[nodiscard]] PwNodeAudioIface* audio() const;

	static PwNodeIface* instance(PwNode* node);

signals:
	void propertiesChanged();
	void readyChanged();

private:
	PwNode* mNode;
	PwNodeAudioIface* audioIface = nullptr;
};

///! A connection between pipewire nodes.
/// Note that there is one link per *channel* of a connection between nodes.
/// You usually want @@PwLinkGroup.
class PwLinkIface: public PwObjectIface {
	Q_OBJECT;
	/// The pipewire object id of the link.
	///
	/// Mainly useful for debugging. you can inspect the link directly
	/// with `pw-cli i <id>`.
	Q_PROPERTY(quint32 id READ id CONSTANT);
	/// The node that is *receiving* information. (the sink)
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* target READ target CONSTANT);
	/// The node that is *sending* information. (the source)
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* source READ source CONSTANT);
	/// The current state of the link.
	///
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
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
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* target READ target CONSTANT);
	/// The node that is *sending* information. (the source)
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* source READ source CONSTANT);
	/// The current state of the link group.
	///
	/// > [!WARNING] This property is invalid unless the node is bound using @@PwObjectTracker.
	Q_PROPERTY(qs::service::pipewire::PwLinkState::Enum state READ state NOTIFY stateChanged);
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
/// PwObjectTracker binds every node given in its @@objects list.
///
/// #### Object Binding
/// By default, pipewire objects are unbound. Unbound objects only have a subset of
/// information available for use or modification. **Binding an object makes all of its
/// properties available for use or modification if applicable.**
///
/// Properties that require their object be bound to use are clearly marked. You do not
/// need to bind the object unless mentioned in the description of the property you
/// want to use.
class PwObjectTracker: public QObject {
	Q_OBJECT;
	/// The list of objects to bind. May contain nulls.
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
