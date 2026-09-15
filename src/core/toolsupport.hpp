#pragma once

#include <qdir.h>

#include "scan.hpp"

namespace qs::core {

class QmlToolingSupport {
public:
	static bool updateTooling(const QDir& configRoot);
	static QString updateMirror(const QDir& configRoot, const QmlScanner& scanner);

private:
	static QString getQmllsConfig();
	static bool lockTooling();
	static bool updateQmllsConfig(const QDir& configRoot, bool create);
	static bool mirrorDir(const QmlScanner& scanner, const QDir& source, const QDir& target);
	static bool mirrorEntry(const QmlScanner& scanner, const QString& path, const QString& target);
	static bool writeMirrorFile(const QString& path, const QByteArray& data);
	static bool linkMirrorEntry(const QString& path, const QString& linkPath);
	static void removeMirrorEntry(const QString& path);
	static inline bool toolingEnabled = false;
	static inline QFile* toolingLock = nullptr;
};

} // namespace qs::core
