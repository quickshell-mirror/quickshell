#pragma once

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qobject.h>
#include <qprocess.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../core/doc.hpp"
#include "../core/reload.hpp"
#include "datastream.hpp"
#include "processcore.hpp"

// Needed when compiling with clang musl-libc++.
// Default include paths contain macros that cause name collisions.
#undef stdout
#undef stderr

///! Child process.
/// #### Example
/// ```qml
/// Process {
///   running: true
///   command: [ "some-command", "arg" ]
///   stdout: @@StdioCollector {
///     onStreamFinished: console.log(`line read: ${this.text}`)
///   }
/// }
/// ```
class Process: public PostReloadHook {
	Q_OBJECT;
	// clang-format off
	/// If the process is currently running. Defaults to false.
	///
	/// Setting this property to true will start the process if command has at least
	/// one element.
	/// Setting it to false will send SIGTERM. To immediately kill the process,
	/// use @@signal() with SIGKILL. The process will be killed when
	/// quickshell dies.
	///
	/// If you want to run the process in a loop, use the onRunningChanged signal handler
	/// to restart the process.
	/// ```qml
	/// Process {
	///   running: true
	///   onRunningChanged: if (!running) running = true
	/// }
	/// ```
	///
	/// > [!NOTE] See @@startDetached() to prevent the process from being killed by Quickshell
	/// > if Quickshell is killed or the configuration is reloaded.
	Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged);
	/// The process ID of the running process or `null` if @@running is false.
	Q_PROPERTY(QVariant processId READ processId NOTIFY processIdChanged);
	/// The command to execute. Each argument is its own string, which means you don't have
	/// to deal with quoting anything.
	///
	/// If the process is already running changing this property will affect the next
	/// started process. If the property has been changed after starting a process it will
	/// return the new value, not the one for the currently running process.
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
	Q_PROPERTY(QList<QString> command READ command WRITE setCommand NOTIFY commandChanged);
	/// The working directory of the process. Defaults to [quickshell's working directory].
	///
	/// If the process is already running changing this property will affect the next
	/// started process. If the property has been changed after starting a process it will
	/// return the new value, not the one for the currently running process.
	///
	/// [quickshell's working directory]: ../../quickshell/quickshell#prop.workingDirectory
	Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged);
	/// Environment of the executed process.
	///
	/// This is a javascript object (json). Environment variables can be added by setting
	/// them to a string and removed by setting them to null (except when @@clearEnvironment is true,
	/// in which case this behavior is inverted, see @@clearEnvironment for details).
	///
	///
	/// ```qml
	/// environment: ({
	///   ADDED: "value",
	///   REMOVED: null,
	///   "i'm different": "value",
	/// })
	/// ```
	///
	/// > [!INFO] You need to wrap the returned object in () otherwise it won't parse due to javascript ambiguity.
	///
	/// If the process is already running changing this property will affect the next
	/// started process. If the property has been changed after starting a process it will
	/// return the new value, not the one for the currently running process.
	Q_PROPERTY(QHash<QString, QVariant> environment READ environment WRITE setEnvironment NOTIFY environmentChanged);
	/// If the process's environment should be cleared prior to applying @@environment.
	/// Defaults to false.
	///
	/// If true, all environment variables will be removed before the @@environment
	/// object is applied, meaning the variables listed will be the only ones visible to the process.
	/// This changes the behavior of `null` to pass in the system value of the variable if present instead
	/// of removing it.
	///
	/// ```qml
	/// clearEnvironment: true
	/// environment: ({
	///   ADDED: "value",
	///   PASSED_FROM_SYSTEM: null,
	/// })
	/// ```
	///
	/// If the process is already running changing this property will affect the next
	/// started process. If the property has been changed after starting a process it will
	/// return the new value, not the one for the currently running process.
	Q_PROPERTY(bool clearEnvironment READ environmentCleared WRITE setEnvironmentCleared NOTIFY environmentClearChanged);
	/// The parser for stdout. If the parser is null the process's stdout channel will be closed
	/// and no further data will be read, even if a new parser is attached.
	Q_PROPERTY(DataStreamParser* stdout READ stdoutParser WRITE setStdoutParser NOTIFY stdoutParserChanged);
	/// The parser for stderr. If the parser is null the process's stdout channel will be closed
	/// and no further data will be read, even if a new parser is attached.
	Q_PROPERTY(DataStreamParser* stderr READ stderrParser WRITE setStderrParser NOTIFY stderrParserChanged);
	/// If stdin is enabled. Defaults to false. If this property is false the process's stdin channel
	/// will be closed and @@write() will do nothing, even if set back to true.
	Q_PROPERTY(bool stdinEnabled READ stdinEnabled WRITE setStdinEnabled NOTIFY stdinEnabledChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit Process(QObject* parent = nullptr);
	~Process() override;
	Q_DISABLE_COPY_MOVE(Process);

	void onPostReload() override;

	// MUST be before exec(ctx) or the other will be called with a default constructed obj.
	QSDOC_HIDE Q_INVOKABLE void exec(QList<QString> command);
	/// Launch a process with the given arguments, stopping any currently running process.
	///
	/// The context parameter can either be a list of command arguments or a JS object with the following fields:
	/// - `command`: A list containing the command and all its arguments. See @@Quickshell.Io.Process.command.
	/// - `environment`: Changes to make to the process environment. See @@Quickshell.Io.Process.environment.
	/// - `clearEnvironment`: Removes all variables from the environment if true.
	/// - `workingDirectory`: The working directory the command should run in.
	///
	/// Passed parameters will change the values currently set in the process.
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
	/// Calling this function is equivalent to running:
	/// ```qml
	/// process.running = false;
	/// process.command = ...
	/// process.environment = ...
	/// process.clearEnvironment = ...
	/// process.workingDirectory = ...
	/// process.running = true;
	/// ```
	Q_INVOKABLE void exec(const qs::io::process::ProcessContext& context);

	/// Sends a signal to the process if @@running is true, otherwise does nothing.
	Q_INVOKABLE void signal(qint32 signal);

	/// Writes to the process's stdin. Does nothing if @@running is false.
	Q_INVOKABLE void write(const QString& data);

	/// Launches an instance of the process detached from Quickshell.
	///
	/// The subprocess will not be tracked, @@running will be false,
	/// and the subprocess will not be killed by Quickshell.
	///
	/// This function is equivalent to @@Quickshell.Quickshell.execDetached().
	Q_INVOKABLE void startDetached();

	[[nodiscard]] bool isRunning() const;
	void setRunning(bool running);

	[[nodiscard]] QVariant processId() const;

	[[nodiscard]] QList<QString> command() const;
	void setCommand(QList<QString> command);

	[[nodiscard]] QString workingDirectory() const;
	void setWorkingDirectory(const QString& workingDirectory);

	[[nodiscard]] QHash<QString, QVariant> environment() const;
	void setEnvironment(QHash<QString, QVariant> environment);

	[[nodiscard]] bool environmentCleared() const;
	void setEnvironmentCleared(bool cleared);

	[[nodiscard]] DataStreamParser* stdoutParser() const;
	void setStdoutParser(DataStreamParser* parser);

	[[nodiscard]] DataStreamParser* stderrParser() const;
	void setStderrParser(DataStreamParser* parser);

	[[nodiscard]] bool stdinEnabled() const;
	void setStdinEnabled(bool enabled);

signals:
	void started();
	void exited(qint32 exitCode, QProcess::ExitStatus exitStatus);

	void runningChanged();
	void processIdChanged();
	void commandChanged();
	void workingDirectoryChanged();
	void environmentChanged();
	void environmentClearChanged();
	void stdoutParserChanged();
	void stderrParserChanged();
	void stdinEnabledChanged();

private slots:
	void onStarted();
	void onFinished(qint32 exitCode, QProcess::ExitStatus exitStatus);
	void onErrorOccurred(QProcess::ProcessError error);
	void onStdoutReadyRead();
	void onStderrReadyRead();
	void onStdoutParserDestroyed();
	void onStderrParserDestroyed();
	void onGlobalWorkingDirectoryChanged();

private:
	void startProcessIfReady();
	void setupEnvironment(QProcess* process);

	QProcess* process = nullptr;
	QList<QString> mCommand;
	QString mWorkingDirectory;
	QHash<QString, QVariant> mEnvironment;
	DataStreamParser* mStdoutParser = nullptr;
	DataStreamParser* mStderrParser = nullptr;
	QByteArray stdoutBuffer;
	QByteArray stderrBuffer;

	bool targetRunning = false;
	bool mStdinEnabled = false;
	bool mClearEnvironment = false;
};
