#include "tag.hpp"

#include <QObject>
#include <QtCore/qtmetamacros.h>
#include <QtTypes>

#include "wayland-dwl-ipc-unstable-v2-client-protocol.h"

namespace qs::dwl {

DwlTag::DwlTag(quint32 index, QObject* parent): QObject(parent), mIndex(index) {}

quint32 DwlTag::index() const { return this->mIndex; }
bool DwlTag::active() const { return this->mActive; }
bool DwlTag::urgent() const { return this->mUrgent; }
quint32 DwlTag::clientCount() const { return this->mClientCount; }
quint32 DwlTag::focusedClient() const { return this->mFocusedClient; }

void DwlTag::updateState(quint32 state, quint32 clients, quint32 focused) {
	const bool newActive = (state & ZDWL_IPC_OUTPUT_V2_TAG_STATE_ACTIVE) != 0;
	const bool newUrgent = (state & ZDWL_IPC_OUTPUT_V2_TAG_STATE_URGENT) != 0;

	if (newActive != this->mActive) {
		this->mActive = newActive;
		emit this->activeChanged();
	}

	if (newUrgent != this->mUrgent) {
		this->mUrgent = newUrgent;
		emit this->urgentChanged();
	}

	if (clients != this->mClientCount) {
		this->mClientCount = clients;
		emit this->clientCountChanged();
	}

	if (focused != this->mFocusedClient) {
		this->mFocusedClient = focused;
		emit this->focusedClientChanged();
	}
}

} // namespace qs::dwl
