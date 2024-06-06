#pragma once

#include <functional>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qhash.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"

namespace qs::hyprland::ipc {

class HyprlandMonitor;
class HyprlandWorkspace;

} // namespace qs::hyprland::ipc

Q_DECLARE_OPAQUE_POINTER(qs::hyprland::ipc::HyprlandWorkspace*);
Q_DECLARE_OPAQUE_POINTER(qs::hyprland::ipc::HyprlandMonitor*);

namespace qs::hyprland::ipc {

///! Live Hyprland IPC event.
/// Live Hyprland IPC event. Holding this object after the
/// signal handler exits is undefined as the event instance
/// is reused.
class HyprlandIpcEvent: public QObject {
	Q_OBJECT;
	/// The name of the event.
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
	[[nodiscard]] HyprlandMonitor* focusedMonitor() const;
	void setFocusedMonitor(HyprlandMonitor* monitor);

	[[nodiscard]] ObjectModel<HyprlandMonitor>* monitors();
	[[nodiscard]] ObjectModel<HyprlandWorkspace>* workspaces();

	// No byId because these preemptively create objects. The given id is set if created.
	HyprlandWorkspace* findWorkspaceByName(const QString& name, bool createIfMissing, qint32 id = 0);
	HyprlandMonitor* findMonitorByName(const QString& name, bool createIfMissing, qint32 id = -1);

	// canCreate avoids making ghost workspaces when the connection races
	void refreshWorkspaces(bool canCreate, bool tryAgain = true);
	void refreshMonitors(bool canCreate, bool tryAgain = true);

	// The last argument may contain commas, so the count is required.
	[[nodiscard]] static QVector<QByteArrayView> parseEventArgs(QByteArrayView event, quint16 count);

signals:
	void connected();
	void rawEvent(HyprlandIpcEvent* event);

	void focusedMonitorChanged();

private slots:
	void eventSocketError(QLocalSocket::LocalSocketError error) const;
	void eventSocketStateChanged(QLocalSocket::LocalSocketState state);
	void eventSocketReady();

	void onFocusedMonitorDestroyed();

private:
	explicit HyprlandIpc();

	void onEvent(HyprlandIpcEvent* event);

	QLocalSocket eventSocket;
	QString mRequestSocketPath;
	QString mEventSocketPath;
	bool valid = false;
	bool requestingMonitors = false;
	bool requestingWorkspaces = false;

	ObjectModel<HyprlandMonitor> mMonitors {this};
	ObjectModel<HyprlandWorkspace> mWorkspaces {this};
	HyprlandMonitor* mFocusedMonitor = nullptr;
	//HyprlandWorkspace* activeWorkspace = nullptr;

	HyprlandIpcEvent event {this};
};

} // namespace qs::hyprland::ipc
