#include "shell.hpp"

#include <qdir.h>
#include <qtmetamacros.h>

void ShellRoot::setConfig(ShellConfig config) {
	this->mConfig = config;

	emit this->configChanged();
}

ShellConfig ShellRoot::config() const { return this->mConfig; }

void ShellConfig::setWorkingDirectory(const QString& workingDirectory) { // NOLINT
	QDir::setCurrent(workingDirectory);
}
