#pragma once

#include <pipewire/link.h>
#include <pipewire/type.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "registry.hpp"

namespace qs::service::pipewire {

///! State of a pipewire link.
/// See @@PwLink.state.
class PwLinkState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : qint8 {
		Error = PW_LINK_STATE_ERROR,
		Unlinked = PW_LINK_STATE_UNLINKED,
		Init = PW_LINK_STATE_INIT,
		Negotiating = PW_LINK_STATE_NEGOTIATING,
		Allocating = PW_LINK_STATE_ALLOCATING,
		Paused = PW_LINK_STATE_PAUSED,
		Active = PW_LINK_STATE_ACTIVE,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(qs::service::pipewire::PwLinkState::Enum value);
};

class PwLink: public PwBindable<pw_link, PW_TYPE_INTERFACE_Link, PW_VERSION_LINK> {
	Q_OBJECT;

public:
	void bindHooks() override;
	void unbindHooks() override;
	void initProps(const spa_dict* props) override;

	[[nodiscard]] quint32 outputNode() const;
	[[nodiscard]] quint32 inputNode() const;
	[[nodiscard]] PwLinkState::Enum state() const;

signals:
	void stateChanged();

private:
	static const pw_link_events EVENTS;
	static void onInfo(void* data, const struct pw_link_info* info);

	void setOutputNode(quint32 outputNode);
	void setInputNode(quint32 inputNode);
	void setState(pw_link_state state);

	SpaHook listener;

	quint32 mOutputNode = 0;
	quint32 mInputNode = 0;
	pw_link_state mState = PW_LINK_STATE_UNLINKED;
};

QDebug operator<<(QDebug debug, const PwLink* link);

class PwLinkGroup: public QObject {
	Q_OBJECT;

public:
	explicit PwLinkGroup(PwLink* firstLink, QObject* parent = nullptr);

	void ref();
	void unref();

	[[nodiscard]] quint32 outputNode() const;
	[[nodiscard]] quint32 inputNode() const;
	[[nodiscard]] PwLinkState::Enum state() const;

	QHash<quint32, PwLink*> links;

	bool tryAddLink(PwLink* link);

signals:
	void stateChanged();

private slots:
	void onLinkRemoved(QObject* object);

private:
	quint32 mOutputNode = 0;
	quint32 mInputNode = 0;
	PwLink* trackedLink = nullptr;
	quint32 refcount = 0;
};

} // namespace qs::service::pipewire
