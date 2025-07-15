#include "desktoputils.hpp"

#include <qdir.h>
#include <qtenvironmentvariables.h>

QList<QString> DesktopUtils::getDesktopDirectories() {
	QList<QString> dataPaths;

	// XDG_DATA_HOME
	QString dataHome = qEnvironmentVariable("XDG_DATA_HOME");
	if (dataHome.isEmpty()) {
		if (qEnvironmentVariableIsSet("HOME")) {
			dataHome = qEnvironmentVariable("HOME") + "/.local/share";
		}
	}
	if (!dataHome.isEmpty()) {
		dataPaths.append(dataHome + "/applications");
	}

	// XDG_DATA_DIRS
	QString dataDirs = qEnvironmentVariable("XDG_DATA_DIRS");
	if (dataDirs.isEmpty()) {
		dataDirs = "/usr/local/share:/usr/share";
	}

	for (const QString& dir: dataDirs.split(':', Qt::SkipEmptyParts)) {
		if (!dir.isEmpty()) {
			// Reduce the amount of files we scan and watch since we only care about .desktop files
			dataPaths.append(dir + "/applications");
		}
	}

	return dataPaths;
}