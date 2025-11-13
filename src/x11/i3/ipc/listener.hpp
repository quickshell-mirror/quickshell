#pragma once

#include <csignal> // NOLINT
#include <utility>

#include <qdir.h>
#include <qhash.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qprocess.h>
#include <qproperty.h>
#include <qqmlinfo.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../../core/doc.hpp"
#include "../../../core/generation.hpp"
#include "../../../core/qmlglobal.hpp"
#include "../../../core/reload.hpp"
#include "connection.hpp"

namespace qs::i3::ipc {

///! I3/Sway IPC event listener
/// #### Example
/// ```qml
/// I3IpcListener {
///   subscriptions: ["input"]
///   onIpcEvent: function (event) {
///     handleInputEvent(event.data)
///   }
/// }
/// ```
class I3IpcListener: public PostReloadHook {
	Q_OBJECT;
	// clang-format off
	/// List of [I3/Sway events](https://man.archlinux.org/man/sway-ipc.7.en#EVENTS) to subscribe to.
	Q_PROPERTY(QList<QString> subscriptions READ subscriptions WRITE setSubscriptions NOTIFY subscriptionsChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit I3IpcListener(QObject* parent = nullptr): PostReloadHook(parent) {}
	~I3IpcListener() override;
	Q_DISABLE_COPY_MOVE(I3IpcListener);

	void onPostReload() override;

	[[nodiscard]] QList<QString> subscriptions() const;
	void setSubscriptions(QList<QString> subscriptions);

signals:
	void ipcEvent(I3IpcEvent* event);
	void subscriptionsChanged();

private:
	void startListening();
	void receiveEvent(I3IpcEvent* event);

	void freeI3Ipc();

	QList<QString> mSubscriptions;
	I3Ipc* i3Ipc = nullptr;
};

} // namespace qs::i3::ipc
