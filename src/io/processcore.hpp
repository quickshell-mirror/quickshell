#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qlist.h>
#include <qprocess.h>
#include <qqmlintegration.h>
#include <qvariant.h>

namespace qs::io::process {

class ProcessContext {
	Q_PROPERTY(QList<QString> command MEMBER command WRITE setCommand);
	Q_PROPERTY(QHash<QString, QVariant> environment MEMBER environment WRITE setEnvironment);
	Q_PROPERTY(bool clearEnvironment MEMBER clearEnvironment WRITE setClearEnvironment);
	Q_PROPERTY(QString workingDirectory MEMBER workingDirectory WRITE setWorkingDirectory);
	Q_PROPERTY(bool unbindStdout MEMBER unbindStdout WRITE setUnbindStdout);
	Q_GADGET;
	QML_STRUCTURED_VALUE;
	QML_VALUE_TYPE(processContext);

public:
	ProcessContext() = default;
	// Making this a Q_INVOKABLE does not work with QML_STRUCTURED_VALUe in Qt 6.9.
	explicit ProcessContext(QList<QString> command): command(std::move(command)), commandSet(true) {}

	void setCommand(QList<QString> command) {
		this->command = std::move(command);
		this->commandSet = true;
	}

	void setEnvironment(QHash<QString, QVariant> environment) {
		this->environment = std::move(environment);
		this->environmentSet = true;
	}

	void setClearEnvironment(bool clearEnvironment) {
		this->clearEnvironment = clearEnvironment;
		this->clearEnvironmentSet = true;
	}

	void setWorkingDirectory(QString workingDirectory) {
		this->workingDirectory = std::move(workingDirectory);
		this->workingDirectorySet = true;
	}

	void setUnbindStdout(bool unbindStdout) { this->unbindStdout = unbindStdout; }

	QList<QString> command;
	QHash<QString, QVariant> environment;
	bool clearEnvironment = false;
	QString workingDirectory;

	bool commandSet : 1 = false;
	bool environmentSet : 1 = false;
	bool clearEnvironmentSet : 1 = false;
	bool workingDirectorySet : 1 = false;
	bool unbindStdout : 1 = true;
};

void setupProcessEnvironment(
    QProcess* process,
    bool clear,
    const QHash<QString, QVariant>& envChanges
);

} // namespace qs::io::process
