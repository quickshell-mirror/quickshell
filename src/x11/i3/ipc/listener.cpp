#include "listener.hpp"
#include <utility>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qdatastream.h>
#include <qobject.h>
#include <qsysinfo.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>

#include "connection.hpp"

namespace qs::i3::ipc {

I3IpcListener::~I3IpcListener() { this->freeI3Ipc(); }

void I3IpcListener::onPostReload() { this->startListening(); }

QList<QString> I3IpcListener::subscriptions() const { return this->mSubscriptions; }
void I3IpcListener::setSubscriptions(QList<QString> subscriptions) {
	if (this->mSubscriptions == subscriptions) return;
	this->mSubscriptions = std::move(subscriptions);

	emit this->subscriptionsChanged();
	this->startListening();
}

void I3IpcListener::startListening() {
	this->freeI3Ipc();
	if (this->mSubscriptions.isEmpty()) return;

	this->i3Ipc = new I3Ipc(this->mSubscriptions);

	// clang-format off
	QObject::connect(this->i3Ipc, &I3Ipc::rawEvent, this, &I3IpcListener::receiveEvent);
	// clang-format on

	this->i3Ipc->connect();
}

void I3IpcListener::receiveEvent(I3IpcEvent* event) { emit this->ipcEvent(event); }

void I3IpcListener::freeI3Ipc() {
	delete this->i3Ipc;
	this->i3Ipc = nullptr;
}

} // namespace qs::i3::ipc
