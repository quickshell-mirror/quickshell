#include "process.hpp"
#include <csignal> // NOLINT
#include <utility>

#include <qdir.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qprocess.h>
#include <qqmlinfo.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../core/common.hpp"
#include "../core/generation.hpp"
#include "../core/qmlglobal.hpp"
#include "datastream.hpp"

Process::Process(QObject* parent): QObject(parent) {
	QObject::connect(
	    QuickshellSettings::instance(),
	    &QuickshellSettings::workingDirectoryChanged,
	    this,
	    &Process::onGlobalWorkingDirectoryChanged
	);
}

bool Process::isRunning() const { return this->process != nullptr; }

void Process::setRunning(bool running) {
	this->targetRunning = running;
	if (running) this->startProcessIfReady();
	else if (this->isRunning()) this->process->terminate();
}

QVariant Process::processId() const {
	if (this->process == nullptr) return QVariant::fromValue(nullptr);
	return QVariant::fromValue(this->process->processId());
}

QList<QString> Process::command() const { return this->mCommand; }

void Process::setCommand(QList<QString> command) {
	if (this->mCommand == command) return;
	this->mCommand = std::move(command);

	auto& cmd = this->mCommand.first();
	if (cmd.startsWith("file://")) {
		cmd = cmd.sliced(7);
	} else if (cmd.startsWith("root://")) {
		cmd = cmd.sliced(7);
		auto& root = EngineGeneration::findObjectGeneration(this)->rootPath;
		cmd = root.filePath(cmd.startsWith('/') ? cmd.sliced(1) : cmd);
	}

	emit this->commandChanged();

	this->startProcessIfReady();
}

QString Process::workingDirectory() const {
	if (this->mWorkingDirectory.isEmpty()) return QDir::current().absolutePath();
	else return this->mWorkingDirectory;
}

void Process::setWorkingDirectory(const QString& workingDirectory) {
	auto absolute =
	    workingDirectory.isEmpty() ? workingDirectory : QDir(workingDirectory).absolutePath();
	if (absolute == this->mWorkingDirectory) return;
	this->mWorkingDirectory = absolute;
	emit this->workingDirectoryChanged();
}

void Process::onGlobalWorkingDirectoryChanged() {
	if (this->mWorkingDirectory.isEmpty()) {
		emit this->workingDirectoryChanged();
	}
}

QMap<QString, QVariant> Process::environment() const { return this->mEnvironment; }

void Process::setEnvironment(QMap<QString, QVariant> environment) {
	if (environment == this->mEnvironment) return;
	this->mEnvironment = std::move(environment);
	emit this->environmentChanged();
}

bool Process::environmentCleared() const { return this->mClearEnvironment; }

void Process::setEnvironmentCleared(bool cleared) {
	if (cleared == this->mClearEnvironment) return;
	this->mClearEnvironment = cleared;
	emit this->environmentClearChanged();
}

DataStreamParser* Process::stdoutParser() const { return this->mStdoutParser; }

void Process::setStdoutParser(DataStreamParser* parser) {
	if (parser == this->mStdoutParser) return;

	if (this->mStdoutParser != nullptr) {
		QObject::disconnect(this->mStdoutParser, nullptr, this, nullptr);

		if (this->process != nullptr) {
			this->process->closeReadChannel(QProcess::StandardOutput);
			this->process->readAllStandardOutput(); // discard
			this->stdoutBuffer.clear();
		}
	}

	this->mStdoutParser = parser;

	if (parser != nullptr) {
		QObject::connect(parser, &QObject::destroyed, this, &Process::onStdoutParserDestroyed);
	}

	emit this->stdoutParserChanged();

	if (parser != nullptr && !this->stdoutBuffer.isEmpty()) {
		parser->parseBytes(this->stdoutBuffer, this->stdoutBuffer);
	}
}

void Process::onStdoutParserDestroyed() {
	this->mStdoutParser = nullptr;
	emit this->stdoutParserChanged();
}

DataStreamParser* Process::stderrParser() const { return this->mStderrParser; }

void Process::setStderrParser(DataStreamParser* parser) {
	if (parser == this->mStderrParser) return;

	if (this->mStderrParser != nullptr) {
		QObject::disconnect(this->mStderrParser, nullptr, this, nullptr);

		if (this->process != nullptr) {
			this->process->closeReadChannel(QProcess::StandardError);
			this->process->readAllStandardError(); // discard
			this->stderrBuffer.clear();
		}
	}

	this->mStderrParser = parser;

	if (parser != nullptr) {
		QObject::connect(parser, &QObject::destroyed, this, &Process::onStderrParserDestroyed);
	}

	emit this->stderrParserChanged();

	if (parser != nullptr && !this->stderrBuffer.isEmpty()) {
		parser->parseBytes(this->stderrBuffer, this->stderrBuffer);
	}
}

void Process::onStderrParserDestroyed() {
	this->mStderrParser = nullptr;
	emit this->stderrParserChanged();
}

bool Process::stdinEnabled() const { return this->mStdinEnabled; }

void Process::setStdinEnabled(bool enabled) {
	if (enabled == this->mStdinEnabled) return;
	this->mStdinEnabled = enabled;

	if (!enabled && this->process != nullptr) {
		this->process->closeWriteChannel();
	}

	emit this->stdinEnabledChanged();
}

void Process::startProcessIfReady() {
	if (this->process != nullptr || !this->targetRunning || this->mCommand.isEmpty()) return;
	this->targetRunning = false;

	auto& cmd = this->mCommand.first();
	auto args = this->mCommand.sliced(1);

	this->process = new QProcess(this);

	// clang-format off
	QObject::connect(this->process, &QProcess::started, this, &Process::onStarted);
	QObject::connect(this->process, &QProcess::finished, this, &Process::onFinished);
	QObject::connect(this->process, &QProcess::errorOccurred, this, &Process::onErrorOccurred);
	QObject::connect(this->process, &QProcess::readyReadStandardOutput, this, &Process::onStdoutReadyRead);
	QObject::connect(this->process, &QProcess::readyReadStandardError, this, &Process::onStderrReadyRead);
	// clang-format on

	this->stdoutBuffer.clear();
	this->stderrBuffer.clear();

	if (this->mStdoutParser == nullptr) this->process->closeReadChannel(QProcess::StandardOutput);
	if (this->mStderrParser == nullptr) this->process->closeReadChannel(QProcess::StandardError);
	if (!this->mStdinEnabled) this->process->closeWriteChannel();

	this->setupEnvironment(this->process);
	this->process->start(cmd, args);
}

void Process::startDetached() {
	if (this->mCommand.isEmpty()) {
		qmlWarning(this) << "Cannot start process as command is empty.";
		return;
	}

	auto& cmd = this->mCommand.first();
	auto args = this->mCommand.sliced(1);

	QProcess process;

	this->setupEnvironment(&process);
	process.setProgram(cmd);
	process.setArguments(args);
	process.startDetached();
}

void Process::setupEnvironment(QProcess* process) {
	if (!this->mWorkingDirectory.isEmpty()) {
		process->setWorkingDirectory(this->mWorkingDirectory);
	}

	const auto& sysenv = qs::Common::INITIAL_ENVIRONMENT;
	auto env = this->mClearEnvironment ? QProcessEnvironment() : sysenv;

	for (auto& name: this->mEnvironment.keys()) {
		auto value = this->mEnvironment.value(name);
		if (!value.isValid()) continue;

		if (this->mClearEnvironment) {
			if (value.isNull()) {
				if (sysenv.contains(name)) env.insert(name, sysenv.value(name));
			} else env.insert(name, value.toString());
		} else {
			if (value.isNull()) env.remove(name);
			else env.insert(name, value.toString());
		}
	}

	process->setProcessEnvironment(env);
}

void Process::onStarted() {
	emit this->processIdChanged();
	emit this->runningChanged();
	emit this->started();
}

void Process::onFinished(qint32 exitCode, QProcess::ExitStatus exitStatus) {
	this->process->deleteLater();
	this->process = nullptr;
	if (this->mStdoutParser) this->mStdoutParser->streamEnded(this->stdoutBuffer);
	if (this->mStderrParser) this->mStderrParser->streamEnded(this->stderrBuffer);
	this->stdoutBuffer.clear();
	this->stderrBuffer.clear();

	emit this->exited(exitCode, exitStatus);
	emit this->runningChanged();
	emit this->processIdChanged();
}

void Process::onErrorOccurred(QProcess::ProcessError error) {
	if (error == QProcess::FailedToStart) { // other cases should be covered by other events
		qWarning() << "Process failed to start, likely because the binary could not be found. Command:"
		           << this->mCommand;
		this->process->deleteLater();
		this->process = nullptr;
		emit this->runningChanged();
	}
}

void Process::onStdoutReadyRead() {
	if (this->mStdoutParser == nullptr) return;
	auto buf = this->process->readAllStandardOutput();
	this->mStdoutParser->parseBytes(buf, this->stdoutBuffer);
}

void Process::onStderrReadyRead() {
	if (this->mStderrParser == nullptr) return;
	auto buf = this->process->readAllStandardError();
	this->mStderrParser->parseBytes(buf, this->stderrBuffer);
}

void Process::signal(qint32 signal) {
	if (this->process == nullptr) return;
	kill(static_cast<qint32>(this->process->processId()), signal); // NOLINT
}

void Process::write(const QString& data) {
	if (this->process == nullptr) return;
	this->process->write(data.toUtf8());
}
