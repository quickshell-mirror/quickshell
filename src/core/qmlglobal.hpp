#pragma once

#include <qclipboard.h>
#include <qcontainerfwd.h>
#include <qhash.h>
#include <qjsengine.h>
#include <qlist.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindowdefs.h>

#include "../io/processcore.hpp"
#include "doc.hpp"
#include "qmlscreen.hpp"

///! Accessor for some options under the Quickshell type.
class QuickshellSettings: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Quickshell's working directory. Defaults to whereever quickshell was launched from.
	Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged);
	/// If true then the configuration will be reloaded whenever any files change.
	/// Defaults to true.
	Q_PROPERTY(bool watchFiles READ watchFiles WRITE setWatchFiles NOTIFY watchFilesChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("singleton");

public:
	QuickshellSettings();

	[[nodiscard]] QString workingDirectory() const;
	void setWorkingDirectory(QString workingDirectory);

	[[nodiscard]] bool watchFiles() const;
	void setWatchFiles(bool watchFiles);

	[[nodiscard]] bool quitOnLastClosed() const;
	void setQuitOnLastClosed(bool exitOnLastClosed);

	static QuickshellSettings* instance();
	static void reset();

signals:
	/// Sent when the last window is closed.
	///
	/// To make the application exit when the last window is closed run `Qt.quit()`.
	void lastWindowClosed();

	void workingDirectoryChanged();
	void watchFilesChanged();

private:
	bool mWatchFiles = true;
};

class QuickshellTracked: public QObject {
	Q_OBJECT;

public:
	QuickshellTracked();

	QVector<QuickshellScreenInfo*> screens;
	QuickshellScreenInfo* screenInfo(QScreen* screen) const;

	static QuickshellTracked* instance();

private slots:
	void updateScreens();

signals:
	void screensChanged();
};

class QuickshellGlobal: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Quickshell's process id.
	Q_PROPERTY(qint32 processId READ processId CONSTANT);
	/// All currently connected screens.
	///
	/// This property updates as connected screens change.
	///
	/// #### Reusing a window on every screen
	/// ```qml
	/// ShellRoot {
	///   Variants {
	///     // see Variants for details
	///     variants: Quickshell.screens
	///     PanelWindow {
	///       property var modelData
	///       screen: modelData
	///     }
	///   }
	/// }
	/// ```
	///
	/// This creates an instance of your window once on every screen.
	/// As screens are added or removed your window will be created or destroyed on those screens.
	Q_PROPERTY(QQmlListProperty<QuickshellScreenInfo> screens READ screens NOTIFY screensChanged);
	/// The full path to the root directory of your shell.
	///
	/// The root directory is the folder containing the entrypoint to your shell, often referred
	/// to as `shell.qml`.
	Q_PROPERTY(QString shellDir READ shellDir CONSTANT);
	/// > [!WARNING] Deprecated: Renamed to @@shellDir for clarity.
	Q_PROPERTY(QString configDir READ configDir CONSTANT);
	/// > [!WARNING] Deprecated: Renamed to @@shellDir for consistency.
	Q_PROPERTY(QString shellRoot READ shellRoot CONSTANT);
	/// Quickshell's working directory. Defaults to whereever quickshell was launched from.
	Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged);
	/// If true then the configuration will be reloaded whenever any files change.
	/// Defaults to true.
	Q_PROPERTY(bool watchFiles READ watchFiles WRITE setWatchFiles NOTIFY watchFilesChanged);
	/// The system clipboard.
	///
	/// > [!WARNING] Under wayland the clipboard will be empty unless a quickshell window is focused.
	Q_PROPERTY(QString clipboardText READ clipboardText WRITE setClipboardText NOTIFY clipboardTextChanged);
	/// The per-shell data directory.
	///
	/// Usually `~/.local/share/quickshell/by-shell/<shell-id>`
	///
	/// Can be overridden using `//@ pragma DataDir $BASE/path` in the root qml file, where `$BASE`
	/// corrosponds to `$XDG_DATA_HOME` (usually `~/.local/share`).
	Q_PROPERTY(QString dataDir READ dataDir CONSTANT);
	/// The per-shell state directory.
	///
	/// Usually `~/.local/state/quickshell/by-shell/<shell-id>`
	///
	/// Can be overridden using `//@ pragma StateDir $BASE/path` in the root qml file, where `$BASE`
	/// corrosponds to `$XDG_STATE_HOME` (usually `~/.local/state`).
	Q_PROPERTY(QString stateDir READ stateDir CONSTANT);
	/// The per-shell cache directory.
	///
	/// Usually `~/.cache/quickshell/by-shell/<shell-id>`
	Q_PROPERTY(QString cacheDir READ cacheDir CONSTANT);
	// clang-format on
	QML_SINGLETON;
	QML_NAMED_ELEMENT(Quickshell);

public:
	[[nodiscard]] qint32 processId() const;

	QQmlListProperty<QuickshellScreenInfo> screens();

	/// Reload the shell.
	///
	/// `hard` - perform a hard reload. If this is false, Quickshell will attempt to reuse windows
	/// that already exist. If true windows will be recreated.
	///
	/// See @@Reloadable for more information on what can be reloaded and how.
	Q_INVOKABLE void reload(bool hard);

	/// Returns the string value of an environment variable or null if it is not set.
	Q_INVOKABLE QVariant env(const QString& variable);

	// MUST be before execDetached(ctx) or the other will be called with a default constructed obj.
	QSDOC_HIDE Q_INVOKABLE static void execDetached(QList<QString> command);
	/// Launch a process detached from Quickshell.
	///
	/// The context parameter can either be a list of command arguments or a JS object with the following fields:
	/// - `command`: A list containing the command and all its arguments. See @@Quickshell.Io.Process.command.
	/// - `environment`: Changes to make to the process environment. See @@Quickshell.Io.Process.environment.
	/// - `clearEnvironment`: Removes all variables from the environment if true.
	/// - `workingDirectory`: The working directory the command should run in.
	///
	/// > [!WARNING] This does not run command in a shell. All arguments to the command
	/// > must be in separate values in the list, e.g. `["echo", "hello"]`
	/// > and not `["echo hello"]`.
	/// >
	/// > Additionally, shell scripts must be run by your shell,
	/// > e.g. `["sh", "script.sh"]` instead of `["script.sh"]` unless the script
	/// > has a shebang.
	///
	/// > [!INFO] You can use `["sh", "-c", <your command>]` to execute your command with
	/// > the system shell.
	///
	/// This function is equivalent to @@Quickshell.Io.Process.startDetached().
	Q_INVOKABLE static void execDetached(const qs::io::process::ProcessContext& context);

	/// Returns a string usable for a @@QtQuick.Image.source for a given system icon.
	///
	/// > [!INFO] By default, icons are loaded from the theme selected by the qt platform theme,
	/// > which means they should match with all other qt applications on your system.
	/// >
	/// > If you want to use a different icon theme, you can put `//@ pragma IconTheme <name>`
	/// > at the top of your root config file or set the `QS_ICON_THEME` variable to the name
	/// > of your icon theme.
	Q_INVOKABLE static QString iconPath(const QString& icon);
	/// Setting the `check` parameter of `iconPath` to true will return an empty string
	/// if the icon does not exist, instead of an image showing a missing texture.
	Q_INVOKABLE static QString iconPath(const QString& icon, bool check);
	/// Setting the `fallback` parameter of `iconPath` will attempt to load the fallback
	/// icon if the requested one could not be loaded.
	Q_INVOKABLE static QString iconPath(const QString& icon, const QString& fallback);
	/// Equivalent to `${Quickshell.configDir}/${path}`
	Q_INVOKABLE [[nodiscard]] QString shellPath(const QString& path) const;
	/// > [!WARNING] Deprecated: Renamed to @@shellPath() for clarity.
	Q_INVOKABLE [[nodiscard]] QString configPath(const QString& path) const;
	/// Equivalent to `${Quickshell.dataDir}/${path}`
	Q_INVOKABLE [[nodiscard]] QString dataPath(const QString& path) const;
	/// Equivalent to `${Quickshell.stateDir}/${path}`
	Q_INVOKABLE [[nodiscard]] QString statePath(const QString& path) const;
	/// Equivalent to `${Quickshell.cacheDir}/${path}`
	Q_INVOKABLE [[nodiscard]] QString cachePath(const QString& path) const;
	/// When called from @@reloadCompleted() or @@reloadFailed(), prevents the
	/// default reload popup from displaying.
	///
	/// The popup can also be blocked by setting `QS_NO_RELOAD_POPUP=1`.
	Q_INVOKABLE void inhibitReloadPopup() { this->mInhibitReloadPopup = true; }

	void clearReloadPopupInhibit() { this->mInhibitReloadPopup = false; }
	[[nodiscard]] bool isReloadPopupInhibited() const { return this->mInhibitReloadPopup; }

	[[nodiscard]] QString shellDir() const;
	[[nodiscard]] QString configDir() const;
	[[nodiscard]] QString shellRoot() const;

	[[nodiscard]] QString workingDirectory() const;
	void setWorkingDirectory(QString workingDirectory);

	[[nodiscard]] bool watchFiles() const;
	void setWatchFiles(bool watchFiles);

	[[nodiscard]] static QString clipboardText();
	static void setClipboardText(const QString& text);

	[[nodiscard]] QString dataDir() const;
	[[nodiscard]] QString stateDir() const;
	[[nodiscard]] QString cacheDir() const;

	static QuickshellGlobal* create(QQmlEngine* engine, QJSEngine* /*unused*/);

signals:
	/// Sent when the last window is closed.
	///
	/// To make the application exit when the last window is closed run `Qt.quit()`.
	void lastWindowClosed();
	/// The reload sequence has completed successfully.
	void reloadCompleted();
	/// The reload sequence has failed.
	void reloadFailed(QString errorString);

	void screensChanged();
	void workingDirectoryChanged();
	void watchFilesChanged();
	void clipboardTextChanged();

private slots:
	void onClipboardChanged(QClipboard::Mode mode);

private:
	QuickshellGlobal(QObject* parent = nullptr);

	bool mInhibitReloadPopup = false;

	static qsizetype screensCount(QQmlListProperty<QuickshellScreenInfo>* prop);
	static QuickshellScreenInfo* screenAt(QQmlListProperty<QuickshellScreenInfo>* prop, qsizetype i);
};
