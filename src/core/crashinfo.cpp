#include "crashinfo.hpp"

#include <qdatastream.h>

QDataStream& operator<<(QDataStream& stream, const InstanceInfo& info) {
	stream << info.configPath << info.shellId << info.launchTime << info.noColor;
	return stream;
}

QDataStream& operator>>(QDataStream& stream, InstanceInfo& info) {
	stream >> info.configPath >> info.shellId >> info.launchTime >> info.noColor;
	return stream;
}

namespace qs::crash {

CrashInfo CrashInfo::INSTANCE = {}; // NOLINT

}
