#pragma once

#include <qdir.h>

#include "scan.hpp"

namespace qs::core {

class QmlToolingSupport {
public:
	static bool updateTooling(const QDir& configRoot, QmlScanner& scanner);

private:
	static QString getQmllsConfig();
	static bool lockTooling();
	static bool updateQmllsConfig(const QDir& configRoot, bool create);
	static void updateToolingFs(QmlScanner& scanner, const QDir& scanDir, const QDir& linkDir);
	static inline bool toolingEnabled = false;
	static inline QFile* toolingLock = nullptr;
};

} // namespace qs::core
