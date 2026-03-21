#pragma once

#include <functional>

#include <qcontainerfwd.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/model.hpp"
#include "output.hpp"
#include "window.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

///! Live niri IPC event.
/// Live niri IPC event. Holding this object after the
/// signal handler exits is undefined as the event instance
/// is reused.
///
/// Emitted by @@Niri.rawEvent(s).
class NiriIpcEvent: public QObject {
	Q_OBJECT;
	/// The type of the event (e.g. "WorkspacesChanged", "WindowClosed").
	Q_PROPERTY(QString type READ type CONSTANT);
	/// The full event data as a JSON object.
	Q_PROPERTY(QJsonObject data READ data CONSTANT);
	QML_NAMED_ELEMENT(NiriEvent);
	QML_UNCREATABLE("NiriIpcEvents cannot be created.");

public:
	explicit NiriIpcEvent(QObject* parent): QObject(parent) {}

	[[nodiscard]] QString type() const { return this->mType; }
	[[nodiscard]] QJsonObject data() const { return this->mData; }

	void set(const QString& type, const QJsonObject& data);
	void reset();

private:
	QString mType;
	QJsonObject mData;
};

class NiriIpc: public QObject {
	Q_OBJECT;

public:
	static NiriIpc* instance();

	[[nodiscard]] QString socketPath() const;

	void makeRequest(
	    const QByteArray& request,
	    const std::function<void(bool, const QJsonObject&)>& callback
	);

	void dispatch(const QStringList& args);

	[[nodiscard]] QBindable<NiriWindow*> bindableFocusedWindow() const {
		return &this->bFocusedWindow;
	}

	[[nodiscard]] QBindable<bool> bindableOverviewActive() const {
		return &this->bOverviewActive;
	}

	[[nodiscard]] QBindable<QStringList> bindableKeyboardLayoutNames() const {
		return &this->bKeyboardLayoutNames;
	}

	[[nodiscard]] QBindable<qint32> bindableCurrentKeyboardLayoutIndex() const {
		return &this->bCurrentKeyboardLayoutIndex;
	}

	[[nodiscard]] ObjectModel<NiriOutput>* outputs();
	[[nodiscard]] ObjectModel<NiriWorkspace>* workspaces();
	[[nodiscard]] ObjectModel<NiriWindow>* windows();

	NiriOutput* findOutputByName(const QString& name);
	NiriWorkspace* findWorkspaceById(qint64 id);
	NiriWindow* findWindowById(qint64 id);

	void refreshOutputs();
	void refreshWorkspaces();
	void refreshWindows();

signals:
	void connected();
	void rawEvent(NiriIpcEvent* event);

	void focusedWindowChanged();
	void overviewActiveChanged();
	void keyboardLayoutsChanged();
	void keyboardLayoutSwitched();

	/// Emitted whenever workspace data changes (items added/removed or properties updated).
	void workspacesUpdated();
	/// Emitted whenever window data changes (items added/removed or properties updated).
	void windowsUpdated();
	/// Emitted whenever output data changes (items added/removed or properties updated).
	void outputsUpdated();

private slots:
	void eventSocketError(QLocalSocket::LocalSocketError error) const;
	void eventSocketStateChanged(QLocalSocket::LocalSocketState state);
	void eventSocketConnected();
	void eventSocketReady();

private:
	explicit NiriIpc();

	void onEvent(const QJsonObject& eventObject);

	void handleWorkspacesChanged(const QJsonObject& data);
	void handleWorkspaceActivated(const QJsonObject& data);
	void handleWorkspaceActiveWindowChanged(const QJsonObject& data);
	void handleWindowOpenedOrChanged(const QJsonObject& data);
	void handleWindowClosed(const QJsonObject& data);
	void handleWindowsChanged(const QJsonObject& data);
	void handleWindowFocusChanged(const QJsonObject& data);
	void handleWindowLayoutsChanged(const QJsonObject& data);
	void handleOverviewOpenedOrClosed(const QJsonObject& data);
	void handleKeyboardLayoutsChanged(const QJsonObject& data);
	void handleKeyboardLayoutSwitched(const QJsonObject& data);

	void updateWorkspacesFromArray(const QJsonArray& workspacesArray);
	void updateOutputsFromObject(const QJsonObject& outputsObject);
	void updateWindowsFromArray(const QJsonArray& windowsArray);

	QString resolveWindowOutput(qint64 workspaceId);

	static bool compareWorkspaces(NiriWorkspace* a, NiriWorkspace* b);

	QLocalSocket eventSocket;
	QString mSocketPath;
	bool valid = false;
	bool requestingOutputs = false;
	bool requestingWorkspaces = false;
	bool requestingWindows = false;

	ObjectModel<NiriOutput> mOutputs {this};
	ObjectModel<NiriWorkspace> mWorkspaces {this};
	ObjectModel<NiriWindow> mWindows {this};

	NiriIpcEvent event {this};

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    NiriWindow*,
	    bFocusedWindow,
	    &NiriIpc::focusedWindowChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    bool,
	    bOverviewActive,
	    &NiriIpc::overviewActiveChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    QStringList,
	    bKeyboardLayoutNames,
	    &NiriIpc::keyboardLayoutsChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    NiriIpc,
	    qint32,
	    bCurrentKeyboardLayoutIndex,
	    &NiriIpc::keyboardLayoutSwitched
	);
};

} // namespace qs::niri::ipc
