#include "shell.hpp"

#include "qmlglobal.hpp"

QuickshellSettings* ShellRoot::settings() const { // NOLINT
	return QuickshellSettings::instance();
}
