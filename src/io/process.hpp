#pragma once

#include <qobject.h>
#include <qprocess.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "datastream.hpp"

///! Child process.
/// #### Example
/// ```qml
/// Process {
///   running: true
///   command: [ "some-command", "arg" ]
///   stdout: SplitParser {
///     onRead: data => console.log(`line read: ${data}`)
///   }
/// }
/// ```
class Process: public QObject {
	Q_OBJECT;
	// clang-format off
	/// If the process is currently running. Defaults to false.
	///
	/// Setting this property to true will start the process if command has at least
	/// one element.
	/// Setting it to false will send SIGTERM. To immediately kill the process,
	/// use [signal](#func.signal) with SIGKILL. The process will be killed when
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
	Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged);
	/// The process ID of the running process or `null` if `running` is false.
	Q_PROPERTY(QVariant pid READ pid NOTIFY pidChanged);
	/// The command to execute.
	///
	/// If the process is already running changing this property will affect the next
	/// started process. If the property has been changed after starting a process it will
	/// return the new value, not the one for the currently running process.
	///
	/// > [!INFO] You can use `["sh", "-c", <your command>]` to execute your command with
	/// > the system shell.
	Q_PROPERTY(QList<QString> command READ command WRITE setCommand NOTIFY commandChanged);
	/// The parser for stdout. If the parser is null the process's stdout channel will be closed
	/// and no further data will be read, even if a new parser is attached.
	Q_PROPERTY(DataStreamParser* stdout READ stdoutParser WRITE setStdoutParser NOTIFY stdoutParserChanged);
	/// The parser for stderr. If the parser is null the process's stdout channel will be closed
	/// and no further data will be read, even if a new parser is attached.
	Q_PROPERTY(DataStreamParser* stderr READ stderrParser WRITE setStderrParser NOTIFY stderrParserChanged);
	/// If stdin is enabled. Defaults to true. If this property is set to false the process's stdin channel
	/// will be closed and [write](#func.write) will do nothing, even if set back to true.
	Q_PROPERTY(bool stdinEnabled READ stdinEnabled WRITE setStdinEnabled NOTIFY stdinEnabledChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit Process(QObject* parent = nullptr): QObject(parent) {}

	/// Sends a signal to the process if `running` is true, otherwise does nothing.
	Q_INVOKABLE void signal(qint32 signal);

	/// Writes to the process's stdin. Does nothing if `running` is false.
	Q_INVOKABLE void write(const QString& data);

	[[nodiscard]] bool isRunning() const;
	void setRunning(bool running);

	[[nodiscard]] QVariant pid() const;

	[[nodiscard]] QList<QString> command() const;
	void setCommand(QList<QString> command);

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
	void pidChanged();
	void commandChanged();
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

private:
	void startProcessIfReady();

	QProcess* process = nullptr;
	QList<QString> mCommand;
	DataStreamParser* mStdoutParser = nullptr;
	DataStreamParser* mStderrParser = nullptr;
	QByteArray stdoutBuffer;
	QByteArray stderrBuffer;

	bool targetRunning = false;
	bool mStdinEnabled = false;
};
