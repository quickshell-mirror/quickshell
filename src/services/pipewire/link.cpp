#include "link.hpp"
#include <cstring>

#include <pipewire/link.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/utils/dict.h>

#include "../../core/logcat.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logLink, "quickshell.service.pipewire.link", QtWarningMsg);
}

QString PwLinkState::toString(Enum value) {
	return QString(pw_link_state_as_string(static_cast<pw_link_state>(value)));
}

void PwLink::bindHooks() {
	pw_link_add_listener(this->proxy(), &this->listener.hook, &PwLink::EVENTS, this);
}

void PwLink::unbindHooks() {
	this->listener.remove();
	this->setState(PW_LINK_STATE_UNLINKED);
}

void PwLink::initProps(const spa_dict* props) {
	qCDebug(logLink) << "Parsing initial SPA props of link" << this;

	const spa_dict_item* item = nullptr;
	spa_dict_for_each(item, props) {
		if (strcmp(item->key, "link.output.node") == 0) {
			auto str = QString(item->value);
			auto ok = false;
			auto value = str.toInt(&ok);
			if (ok) this->setOutputNode(value);
			else {
				qCWarning(logLink) << "Could not parse link.output.node for" << this << ":" << item->value;
			}
		} else if (strcmp(item->key, "link.input.node") == 0) {
			auto str = QString(item->value);
			auto ok = false;
			auto value = str.toInt(&ok);
			if (ok) this->setInputNode(value);
			else {
				qCWarning(logLink) << "Could not parse link.input.node for" << this << ":" << item->value;
			}
		}
	}
}

const pw_link_events PwLink::EVENTS = {
    .version = PW_VERSION_LINK_EVENTS,
    .info = &PwLink::onInfo,
};

void PwLink::onInfo(void* data, const struct pw_link_info* info) {
	auto* self = static_cast<PwLink*>(data);
	qCDebug(logLink) << "Got link info update for" << self << "with mask" << info->change_mask;
	self->setOutputNode(info->output_node_id);
	self->setInputNode(info->input_node_id);

	if ((info->change_mask & PW_LINK_CHANGE_MASK_STATE) != 0) {
		self->setState(info->state);
	}
}

quint32 PwLink::outputNode() const { return this->mOutputNode; }
quint32 PwLink::inputNode() const { return this->mInputNode; }
PwLinkState::Enum PwLink::state() const { return static_cast<PwLinkState::Enum>(this->mState); }

void PwLink::setOutputNode(quint32 outputNode) {
	if (outputNode == this->mOutputNode) return;

	if (this->mOutputNode != 0) {
		qCWarning(logLink) << "Got unexpected output node update for" << this << "to" << outputNode;
	}

	this->mOutputNode = outputNode;
	qCDebug(logLink) << "Updated output node of" << this;
}

void PwLink::setInputNode(quint32 inputNode) {
	if (inputNode == this->mInputNode) return;

	if (this->mInputNode != 0) {
		qCWarning(logLink) << "Got unexpected input node update for" << this << "to" << inputNode;
	}

	this->mInputNode = inputNode;
	qCDebug(logLink) << "Updated input node of" << this;
}

void PwLink::setState(pw_link_state state) {
	if (state == this->mState) return;

	this->mState = state;
	qCDebug(logLink) << "Updated state of" << this;
	emit this->stateChanged();
}

QDebug operator<<(QDebug debug, const PwLink* link) {
	if (link == nullptr) {
		debug << "PwLink(0x0)";
	} else {
		auto saver = QDebugStateSaver(debug);
		debug.nospace() << "PwLink(" << link->outputNode() << " -> " << link->inputNode() << ", "
		                << static_cast<const void*>(link) << ", id=";
		link->debugId(debug);
		debug << ", state=" << link->state() << ')';
	}

	return debug;
}

PwLinkGroup::PwLinkGroup(PwLink* firstLink, QObject* parent)
    : QObject(parent)
    , mOutputNode(firstLink->outputNode())
    , mInputNode(firstLink->inputNode()) {
	this->tryAddLink(firstLink);
}

void PwLinkGroup::ref() {
	this->refcount++;

	if (this->refcount == 1) {
		this->trackedLink = *this->links.begin();
		this->trackedLink->ref();
		QObject::connect(this->trackedLink, &PwLink::stateChanged, this, &PwLinkGroup::stateChanged);
		emit this->stateChanged();
	}
}

void PwLinkGroup::unref() {
	if (this->refcount == 0) return;
	this->refcount--;

	if (this->refcount == 0) {
		this->trackedLink->unref();
		this->trackedLink = nullptr;
		emit this->stateChanged();
	}
}

quint32 PwLinkGroup::outputNode() const { return this->mOutputNode; }

quint32 PwLinkGroup::inputNode() const { return this->mInputNode; }

PwLinkState::Enum PwLinkGroup::state() const {
	if (this->trackedLink == nullptr) {
		return PwLinkState::Unlinked;
	} else {
		return this->trackedLink->state();
	}
}

bool PwLinkGroup::tryAddLink(PwLink* link) {
	if (link->outputNode() != this->mOutputNode || link->inputNode() != this->mInputNode)
		return false;

	this->links.insert(link->id, link);
	QObject::connect(link, &PwBindableObject::destroying, this, &PwLinkGroup::onLinkRemoved);
	return true;
}

void PwLinkGroup::onLinkRemoved(QObject* object) {
	auto* link = static_cast<PwLink*>(object); // NOLINT
	this->links.remove(link->id);

	if (this->links.empty()) {
		delete this;
	} else if (link == this->trackedLink) {
		this->trackedLink = *this->links.begin();
		QObject::connect(this->trackedLink, &PwLink::stateChanged, this, &PwLinkGroup::stateChanged);
		emit this->stateChanged();
	}
}

} // namespace qs::service::pipewire
