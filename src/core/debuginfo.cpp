#include "debuginfo.hpp"
#include <array>
#include <cstring>
#include <string_view>

#include <fcntl.h>
#include <qconfig.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfloat16.h>
#include <qhashfunctions.h>
#include <qscopeguard.h>
#include <qtversion.h>
#include <unistd.h>
#include <xf86drm.h>

#include "build.hpp"

extern char** environ; // NOLINT

namespace qs::debuginfo {

QString qsVersion() {
	return QS_VERSION " (revision " GIT_REVISION ", distributed by " DISTRIBUTOR ")";
}

QString qtVersion() { return qVersion() % QStringLiteral(" (built against " QT_VERSION_STR ")"); }

QString gpuInfo() {
	auto deviceCount = drmGetDevices2(0, nullptr, 0);
	if (deviceCount < 0) return "Failed to get DRM device count: " % QString::number(deviceCount);
	auto* devices = new drmDevicePtr[deviceCount];
	auto devicesArrayGuard = qScopeGuard([&] { delete[] devices; });
	auto r = drmGetDevices2(0, devices, deviceCount);
	if (deviceCount < 0) return "Failed to get DRM devices: " % QString::number(r);
	auto devicesGuard = qScopeGuard([&] {
		for (auto i = 0; i != deviceCount; ++i) drmFreeDevice(&devices[i]); // NOLINT
	});

	QString info;
	auto stream = QTextStream(&info);

	for (auto i = 0; i != deviceCount; ++i) {
		auto* device = devices[i]; // NOLINT

		int deviceNodeType = -1;
		if (device->available_nodes & (1 << DRM_NODE_RENDER)) deviceNodeType = DRM_NODE_RENDER;
		else if (device->available_nodes & (1 << DRM_NODE_PRIMARY)) deviceNodeType = DRM_NODE_PRIMARY;

		if (deviceNodeType == -1) continue;

		auto* deviceNode = device->nodes[DRM_NODE_RENDER]; // NOLINT

		auto driver = [&]() -> QString {
			auto fd = open(deviceNode, O_RDWR | O_CLOEXEC);
			if (fd == -1) return "<failed to open device node>";
			auto fdGuard = qScopeGuard([&] { close(fd); });
			auto* ver = drmGetVersion(fd);
			if (!ver) return "<drmGetVersion failed>";
			auto verGuard = qScopeGuard([&] { drmFreeVersion(ver); });

			// clang-format off
			return QString(ver->name)
				% ' ' % QString::number(ver->version_major)
				% '.' % QString::number(ver->version_minor)
				% '.' % QString::number(ver->version_patchlevel)
				% " (" % ver->desc % ')';
			// clang-format on
		}();

		QString product = "unknown";
		QString address = "unknown";

		auto hex = [](int num, int pad) { return QString::number(num, 16).rightJustified(pad, '0'); };

		switch (device->bustype) {
		case DRM_BUS_PCI: {
			auto* b = device->businfo.pci;
			auto* d = device->deviceinfo.pci;
			address = "PCI " % hex(b->bus, 2) % ':' % hex(b->dev, 2) % '.' % hex(b->func, 1);
			product = hex(d->vendor_id, 4) % ':' % hex(d->device_id, 4);
		} break;
		case DRM_BUS_USB: {
			auto* b = device->businfo.usb;
			auto* d = device->deviceinfo.usb;
			address = "USB " % QString::number(b->bus) % ':' % QString::number(b->dev);
			product = hex(d->vendor, 4) % ':' % hex(d->product, 4);
		} break;
		default: break;
		}

		stream << "GPU " << deviceNode << "\n  Driver: " << driver << "\n  Model: " << product
		       << "\n  Address: " << address << '\n';
	}

	return info;
}

QString systemInfo() {
	QString info;
	auto stream = QTextStream(&info);

	stream << gpuInfo() << '\n';

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

QString envInfo() {
	QString info;
	auto stream = QTextStream(&info);

	for (auto** envp = environ; *envp != nullptr; ++envp) { // NOLINT
		auto prefixes = std::array<std::string_view, 5> {
		    "QS_",
		    "QT_",
		    "QML_",
		    "QML2_",
		    "QSG_",
		};

		for (const auto& prefix: prefixes) {
			if (strncmp(prefix.data(), *envp, prefix.length()) == 0) goto print;
		}
		continue;

	print:
		stream << *envp << '\n';
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

	stream << "\n===== Environment (trimmed) =====\n";
	stream << envInfo();

	return info;
}

} // namespace qs::debuginfo
