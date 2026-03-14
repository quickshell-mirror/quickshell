#include "debuginfo.hpp"

#include <fcntl.h>
#include <qconfig.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfloat16.h>
#include <qhashfunctions.h>
#include <qtversion.h>
#include <unistd.h>

#include "build.hpp"

namespace qs::debuginfo {

QString qsVersion() {
	return QS_VERSION " (revision " GIT_REVISION ", distributed by " DISTRIBUTOR ")";
}

QString qtVersion() { return qVersion() % QStringLiteral(" (built against " QT_VERSION_STR ")"); }

QString systemInfo() {
	QString info;
	auto stream = QTextStream(&info);

	stream << "/etc/os-release:";
	auto osReleaseFile = QFile("/etc/os-release");
	if (osReleaseFile.open(QFile::ReadOnly)) {
		stream << '\n' << osReleaseFile.readAll() << '\n';
		osReleaseFile.close();
	} else {
		stream << "FAILED TO OPEN\n";
	}

	stream << "/etc/lsb-release:";
	auto lsbReleaseFile = QFile("/etc/lsb-release");
	if (lsbReleaseFile.open(QFile::ReadOnly)) {
		stream << '\n' << lsbReleaseFile.readAll();
		lsbReleaseFile.close();
	} else {
		stream << "FAILED TO OPEN\n";
	}

	return info;
}

QString combinedInfo() {
	QString info;
	auto stream = QTextStream(&info);

	stream << "===== Version Information =====\n";
	stream << "Quickshell: " << qsVersion() << '\n';
	stream << "Qt: " << qtVersion() << '\n';

	stream << "\n===== Build Information =====\n";
	stream << "Build Type: " << BUILD_TYPE << '\n';
	stream << "Compiler: " << COMPILER << '\n';
	stream << "Compile Flags: " << COMPILE_FLAGS << '\n';
	stream << "Configuration:\n" << BUILD_CONFIGURATION << '\n';

	stream << "\n===== System Information =====\n";
	stream << systemInfo();

	return info;
}

} // namespace qs::debuginfo
