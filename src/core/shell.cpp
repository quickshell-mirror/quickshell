#include "shell.hpp"

#include <qtmetamacros.h>

void ShellRoot::setConfig(ShellConfig config) {
	this->mConfig = config;

	emit this->configChanged();
}

ShellConfig ShellRoot::config() const { return this->mConfig; }
