#pragma once

#include <functional>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qhash.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "../../../wayland/toplevel_management/handle.hpp"

namespace qs::hyprland::ipc {

class HyprlandMonitor;
class HyprlandWorkspace;
class HyprlandToplevel;

} // namespace qs::hyprland::ipc

Q_DECLARE_OPAQUE_POINTER(qs::hyprland::ipc::HyprlandWorkspace*);
Q_DECLARE_OPAQUE_POINTER(qs::hyprland::ipc::HyprlandMonitor*);
Q_DECLARE_OPAQUE_POINTER(qs::hyprland::ipc::HyprlandToplevel*);

namespace qs::hyprland::ipc {

///! Live Hyprland IPC event.
/// Live Hyprland IPC event. Holding this object after the
/// signal handler exits is undefined as the event instance
/// is reused.
///
/// Emitted by @@Hyprland.rawEvent(s).
class HyprlandIpcEvent: public QObject {
	Q_OBJECT;
	/// The name of the event.
	///
	/// See [Hyprland Wiki: IPC](https://wiki.hyprland.org/IPC/) for a list of events.
	Q_PROPERTY(QString name READ nameStr CONSTANT);
	/// The unparsed data of the event.
	Q_PROPERTY(QString data READ dataStr CONSTANT);
	QML_NAMED_ELEMENT(HyprlandEvent);
	QML_UNCREATABLE("HyprlandIpcEvents cannot be created.");

public:
	HyprlandIpcEvent(QObject* parent): QObject(parent) {}

	/// Parse this event with a known number of arguments.
	///
	/// Argument count is required as some events can contain commas
	/// in the last argument, which can be ignored as long as the count is known.
	Q_INVOKABLE [[nodiscard]] QVector<QString> parse(qint32 argumentCount) const;
	[[nodiscard]] QVector<QByteArrayView> parseView(qint32 argumentCount) const;

	[[nodiscard]] QString nameStr() const;
	[[nodiscard]] QString dataStr() const;

	void reset();
	QByteArrayView name;
	QByteArrayView data;
};

class HyprlandIpc: public QObject {
	Q_OBJECT;

public:
	static HyprlandIpc* instance();

	[[nodiscard]] QString requestSocketPath() const;
	[[nodiscard]] QString eventSocketPath() const;

	void
	makeRequest(const QByteArray& request, const std::function<void(bool, QByteArray)>& callback);
	void dispatch(const QString& request);

	[[nodiscard]] HyprlandMonitor* monitorFor(QuickshellScreenInfo* screen);

	[[nodiscard]] QBindable<HyprlandMonitor*> bindableFocusedMonitor() const {
		return &this->bFocusedMonitor;
	}

	[[nodiscard]] QBindable<HyprlandWorkspace*> bindableFocusedWorkspace() const {
		return &this->bFocusedWorkspace;
	}

	[[nodiscard]] QBindable<HyprlandToplevel*> bindableActiveToplevel() const {
		return &this->bActiveToplevel;
	}

	void setFocusedMonitor(HyprlandMonitor* monitor);

	[[nodiscard]] ObjectModel<HyprlandMonitor>* monitors();
	[[nodiscard]] ObjectModel<HyprlandWorkspace>* workspaces();
	[[nodiscard]] ObjectModel<HyprlandToplevel>* toplevels();

	// No byId because these preemptively create objects. The given id is set if created.
	HyprlandWorkspace* findWorkspaceByName(const QString& name, bool createIfMissing, qint32 id = -1);
	HyprlandMonitor* findMonitorByName(const QString& name, bool createIfMissing, qint32 id = -1);
	HyprlandToplevel* findToplevelByAddress(quint64 address, bool createIfMissing);

	// canCreate avoids making ghost workspaces when the connection races
	void refreshWorkspaces(bool canCreate);
	void refreshMonitors(bool canCreate);
	void refreshToplevels();

	// The last argument may contain commas, so the count is required.
	[[nodiscard]] static QVector<QByteArrayView> parseEventArgs(QByteArrayView event, quint16 count);

signals:
	void connected();
	void rawEvent(HyprlandIpcEvent* event);

	void focusedMonitorChanged();
	void focusedWorkspaceChanged();
	void activeToplevelChanged();

private slots:
	void eventSocketError(QLocalSocket::LocalSocketError error) const;
	void eventSocketStateChanged(QLocalSocket::LocalSocketState state);
	void eventSocketReady();

	void toplevelAddressed(
	    qs::wayland::toplevel_management::impl::ToplevelHandle* handle,
	    quint64 address
	);

	void onFocusedMonitorDestroyed();

private:
	explicit HyprlandIpc();

	void onEvent(HyprlandIpcEvent* event);

	static bool compareWorkspaces(HyprlandWorkspace* a, HyprlandWorkspace* b);

	QLocalSocket eventSocket;
	QString mRequestSocketPath;
	QString mEventSocketPath;
	bool valid = false;
	bool requestingMonitors = false;
	bool requestingWorkspaces = false;
	bool requestingToplevels = false;
	bool monitorsRequested = false;

	ObjectModel<HyprlandMonitor> mMonitors {this};
	ObjectModel<HyprlandWorkspace> mWorkspaces {this};
	ObjectModel<HyprlandToplevel> mToplevels {this};

	HyprlandIpcEvent event {this};

	Q_OBJECT_BINDABLE_PROPERTY(
	    HyprlandIpc,
	    HyprlandMonitor*,
	    bFocusedMonitor,
	    &HyprlandIpc::focusedMonitorChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    HyprlandIpc,
	    HyprlandWorkspace*,
	    bFocusedWorkspace,
	    &HyprlandIpc::focusedWorkspaceChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    HyprlandIpc,
	    HyprlandToplevel*,
	    bActiveToplevel,
	    &HyprlandIpc::activeToplevelChanged
	);
};

} // namespace qs::hyprland::ipc
