#pragma once

#include <functional>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qhash.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qstringlist.h>
#include <qtmetamacros.h>
#include <qtimer.h>
#include <qtypes.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "../../../core/streamreader.hpp"

namespace qs::niri::ipc {

class NiriCast;
class NiriMonitor;
class NiriWindow;
class NiriWorkspace;

} // namespace qs::niri::ipc

Q_DECLARE_OPAQUE_POINTER(qs::niri::ipc::NiriWorkspace*);
Q_DECLARE_OPAQUE_POINTER(qs::niri::ipc::NiriMonitor*);
Q_DECLARE_OPAQUE_POINTER(qs::niri::ipc::NiriWindow*);
Q_DECLARE_OPAQUE_POINTER(qs::niri::ipc::NiriCast*);

namespace qs::niri::ipc {

///! Live niri IPC event.
/// Live niri IPC event. Holding this object after the
/// signal handler exits is undefined as the event instance
/// is reused.
///
/// Emitted by @@Niri.rawEvent(s).
class NiriIpcEvent: public QObject {
	Q_OBJECT;

	/// The name of the event.
	///
	/// See the [niri-ipc documentation](https://niri-wm.github.io/niri/niri_ipc/enum.Event.html)
	/// for a list of events.
	Q_PROPERTY(QString name READ nameStr CONSTANT);
	/// The unparsed data of the event, as a JSON string.
	Q_PROPERTY(QString data READ dataStr CONSTANT);
	QML_NAMED_ELEMENT(NiriEvent);
	QML_UNCREATABLE("NiriIpcEvents cannot be created.");

public:
	NiriIpcEvent(QObject* parent): QObject(parent) {}

	/// Parse the event data as a javascript object.
	Q_INVOKABLE [[nodiscard]] QVariantMap parse() const;

	[[nodiscard]] QString nameStr() const;
	[[nodiscard]] QString dataStr() const;

	QByteArray name;
	QByteArray data;
};

class NiriIpc: public QObject {
	Q_OBJECT;

public:
	static NiriIpc* instance();

	[[nodiscard]] QString socketPath() const;

	void
	makeRequest(const QByteArray& request, const std::function<void(bool, QByteArray)>& callback);
	void dispatch(const QString& request);

	[[nodiscard]] NiriMonitor* monitorFor(QuickshellScreenInfo* screen);

	[[nodiscard]] QBindable<NiriMonitor*> bindableFocusedMonitor() const {
		return &this->bFocusedMonitor;
	}

	[[nodiscard]] QBindable<NiriWorkspace*> bindableFocusedWorkspace() const {
		return &this->bFocusedWorkspace;
	}

	[[nodiscard]] QBindable<NiriWindow*> bindableActiveWindow() const { return &this->bActiveWindow; }

	[[nodiscard]] QBindable<bool> bindableOverviewOpen() const { return &this->bOverviewOpen; }

	[[nodiscard]] QBindable<qint32> bindableCurrentKeyboardLayout() const {
		return &this->bCurrentKeyboardLayout;
	}

	[[nodiscard]] QStringList keyboardLayouts() const { return this->mKeyboardLayouts; }

	void setFocusedMonitor(NiriMonitor* monitor);
	void setActiveWindow(NiriWindow* window);

	[[nodiscard]] ObjectModel<NiriMonitor>* monitors();
	[[nodiscard]] ObjectModel<NiriWorkspace>* workspaces();
	[[nodiscard]] ObjectModel<NiriWindow>* windows();
	[[nodiscard]] ObjectModel<NiriCast>* casts();

	NiriMonitor* findMonitorByName(const QString& name, bool createIfMissing);
	NiriWorkspace* findWorkspaceById(qint64 id);
	NiriWindow* findWindowById(qint64 id);
	NiriCast* findCastByStreamId(qint64 streamId);

	void refreshMonitors();
	void refreshWorkspaces();
	void refreshWindows();
	void refreshCasts();

signals:
	void connected();
	void rawEvent(NiriIpcEvent* event);

	/// Emitted when niri reloads its configuration. Also emitted once when
	/// the event stream connects, indicating the last config load attempt.
	void configLoaded(bool failed);
	/// Emitted when niri captures a screenshot. Empty path when the screenshot
	/// was only copied to the clipboard.
	void screenshotCaptured(const QString& path);

	void focusedMonitorChanged();
	void focusedWorkspaceChanged();
	void activeWindowChanged();
	void overviewOpenChanged();
	void keyboardLayoutsChanged();
	void currentKeyboardLayoutChanged();

private slots:
	void socketError(QLocalSocket::LocalSocketError error) const;
	void socketStateChanged(QLocalSocket::LocalSocketState state);
	void socketReady();

	void onFocusedMonitorDestroyed();

private:
	explicit NiriIpc();

	void onEvent(NiriIpcEvent* event);

	static bool compareWorkspaces(NiriWorkspace* a, NiriWorkspace* b);

	void updateWorkspaces(const QVariantList& workspaces);
	void updateMonitors(const QVariantList& monitors);
	void updateWindows(const QVariantList& windows);
	void updateCasts(const QVariantList& casts);

	QLocalSocket eventSocket;
	StreamReader eventReader;
	QTimer reconnectTimer;
	QString mSocketPath;
	bool valid = false;
	bool requestingMonitors = false;
	bool requestingWorkspaces = false;
	bool requestingWindows = false;
	bool requestingCasts = false;
	bool monitorsRequested = false;

	ObjectModel<NiriMonitor> mMonitors {this};
	ObjectModel<NiriWorkspace> mWorkspaces {this};
	ObjectModel<NiriWindow> mWindows {this};
	ObjectModel<NiriCast> mCasts {this};

	QStringList mKeyboardLayouts;

	NiriIpcEvent event {this};

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    NiriMonitor*,
	    bFocusedMonitor,
	    &NiriIpc::focusedMonitorChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    NiriWorkspace*,
	    bFocusedWorkspace,
	    &NiriIpc::focusedWorkspaceChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(NiriIpc, NiriWindow*, bActiveWindow, &NiriIpc::activeWindowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriIpc, bool, bOverviewOpen, &NiriIpc::overviewOpenChanged);

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    qint32,
	    bCurrentKeyboardLayout,
	    &NiriIpc::currentKeyboardLayoutChanged
	);
};

} // namespace qs::niri::ipc
