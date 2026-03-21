#pragma once

#include <qstring.h>

#include "core.hpp"
#include "defaults.hpp"
#include "registry.hpp"

class QFileSystemWatcher;

namespace qs::service::pipewire {

class PwConnection: public QObject {
	Q_OBJECT;

public:
	explicit PwConnection(QObject* parent = nullptr);

	PwRegistry registry;
	PwDefaultTracker defaults {&this->registry};

	static PwConnection* instance();

private:
	static QString resolveRuntimeDir();

	void beginReconnect();
	bool tryConnect(bool retry);
	void startSocketWatcher();
	void stopSocketWatcher();

private slots:
	void queueFatalError();
	void onFatalError();
	void onRuntimeDirChanged(const QString& path);

private:
	QString runtimeDir;
	QFileSystemWatcher* socketWatcher = nullptr;
	bool fatalErrorQueued = false;

	// init/destroy order is important. do not rearrange.
	PwCore core;
};

} // namespace qs::service::pipewire
