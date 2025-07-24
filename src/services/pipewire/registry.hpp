#pragma once

#include <pipewire/core.h>
#include <pipewire/proxy.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../core/util.hpp"
#include "core.hpp"

namespace qs::service::pipewire {

QS_DECLARE_LOGGING_CATEGORY(logRegistry);

class PwRegistry;
class PwMetadata;
class PwNode;
class PwLink;
class PwDevice;
class PwLinkGroup;

class PwBindableObject: public QObject {
	Q_OBJECT;

public:
	PwBindableObject() = default;
	~PwBindableObject() override;
	Q_DISABLE_COPY_MOVE(PwBindableObject);

	// constructors and destructors can't do virtual calls.
	virtual void init(PwRegistry* registry, quint32 id, quint32 perms);
	virtual void initProps(const spa_dict* /*props*/) {}
	virtual void safeDestroy();

	quint32 id = 0;
	quint32 perms = 0;

	void debugId(QDebug& debug) const;
	void ref();
	void unref();

signals:
	// goes with safeDestroy
	void destroying(PwBindableObject* self);

protected:
	void registryBind(const char* interface, quint32 version);
	virtual void bind();
	void unbind();
	virtual void bindHooks() {}
	virtual void unbindHooks() {}

	quint32 refcount = 0;
	pw_proxy* object = nullptr;
	PwRegistry* registry = nullptr;
};

QDebug operator<<(QDebug debug, const PwBindableObject* object);

template <typename T, StringLiteral INTERFACE, quint32 VERSION>
class PwBindable: public PwBindableObject {
public:
	T* proxy() { return reinterpret_cast<T*>(this->object); }

protected:
	void bind() override {
		if (this->object != nullptr) return;
		this->registryBind(INTERFACE, VERSION);
		this->PwBindableObject::bind();
	}

	friend class PwRegistry;
};

class PwBindableObjectRef: public QObject {
	Q_OBJECT;

public:
	explicit PwBindableObjectRef(PwBindableObject* object = nullptr);
	~PwBindableObjectRef() override;
	Q_DISABLE_COPY_MOVE(PwBindableObjectRef);

signals:
	void objectDestroyed();

private slots:
	void onObjectDestroyed();

protected:
	void setObject(PwBindableObject* object);

	PwBindableObject* mObject = nullptr;
};

template <typename T>
class PwBindableRef: public PwBindableObjectRef {
public:
	explicit PwBindableRef(T* object = nullptr): PwBindableObjectRef(object) {}

	T* object() { return static_cast<T*>(this->mObject); }
	void setObject(T* object) { this->PwBindableObjectRef::setObject(object); }
};

class PwRegistry
    : public QObject
    , public PwObject<pw_registry> {
	Q_OBJECT;

public:
	void init(PwCore& core);

	[[nodiscard]] bool isInitialized() const { return this->initState == InitState::Done; }

	//QHash<quint32, PwClient*> clients;
	QHash<quint32, PwMetadata*> metadata;
	QHash<quint32, PwNode*> nodes;
	QHash<quint32, PwDevice*> devices;
	QHash<quint32, PwLink*> links;
	QVector<PwLinkGroup*> linkGroups;

	PwCore* core = nullptr;

	[[nodiscard]] PwNode* findNodeByName(QStringView name) const;

signals:
	void nodeAdded(PwNode* node);
	void linkAdded(PwLink* link);
	void linkGroupAdded(PwLinkGroup* group);
	void metadataAdded(PwMetadata* metadata);
	void initialized();

private slots:
	void onLinkGroupDestroyed(QObject* object);
	void onCoreSync(quint32 id, qint32 seq);

private:
	static const pw_registry_events EVENTS;

	static void onGlobal(
	    void* data,
	    quint32 id,
	    quint32 permissions,
	    const char* type,
	    quint32 version,
	    const spa_dict* props
	);

	static void onGlobalRemoved(void* data, quint32 id);

	void addLinkToGroup(PwLink* link);

	enum class InitState : quint8 {
		SendingObjects,
		Binding,
		Done
	} initState = InitState::SendingObjects;

	qint32 coreSyncSeq = 0;
	SpaHook listener;
};

} // namespace qs::service::pipewire
