#include "connection.hpp"

#include <qdir.h>
#include <qfilesystemwatcher.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <unistd.h>

#include "../../core/logcat.hpp"
#include "core.hpp"

namespace qs::service::pipewire {

namespace {
QS_LOGGING_CATEGORY(logConnection, "quickshell.service.pipewire.connection", QtWarningMsg);
}

PwConnection::PwConnection(QObject* parent): QObject(parent) {
	this->runtimeDir = PwConnection::resolveRuntimeDir();

	QObject::connect(&this->core, &PwCore::fatalError, this, &PwConnection::queueFatalError);

	if (!this->tryConnect(false)
	    && qEnvironmentVariableIntValue("QS_PIPEWIRE_IMMEDIATE_RECONNECT") == 1)
	{
		this->beginReconnect();
	}
}

QString PwConnection::resolveRuntimeDir() {
	auto runtimeDir = qEnvironmentVariable("PIPEWIRE_RUNTIME_DIR");
	if (runtimeDir.isEmpty()) {
		runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
	}

	if (runtimeDir.isEmpty()) {
		runtimeDir = QString("/run/user/%1").arg(getuid());
	}

	return runtimeDir;
}

void PwConnection::beginReconnect() {
	if (this->core.isValid()) {
		this->stopSocketWatcher();
		return;
	}

	if (!qEnvironmentVariableIsEmpty("PIPEWIRE_REMOTE")) return;

	if (this->runtimeDir.isEmpty()) {
		qCWarning(
		    logConnection
		) << "Cannot watch runtime dir for pipewire reconnects: runtime dir is empty.";
		return;
	}

	this->startSocketWatcher();
	this->tryConnect(true);
}

bool PwConnection::tryConnect(bool retry) {
	if (this->core.isValid()) return true;

	qCDebug(logConnection) << "Attempting reconnect...";
	if (!this->core.start(retry)) {
		return false;
	}

	qCInfo(logConnection) << "Connection established";
	this->stopSocketWatcher();

	this->registry.init(this->core);
	return true;
}

void PwConnection::startSocketWatcher() {
	if (this->socketWatcher != nullptr) return;
	if (!qEnvironmentVariableIsEmpty("PIPEWIRE_REMOTE")) return;

	auto dir = QDir(this->runtimeDir);
	if (!dir.exists()) {
		qCWarning(logConnection) << "Cannot wait for a new pipewire socket, runtime dir does not exist:"
		                         << this->runtimeDir;
		return;
	}

	this->socketWatcher = new QFileSystemWatcher(this);
	this->socketWatcher->addPath(this->runtimeDir);

	QObject::connect(
	    this->socketWatcher,
	    &QFileSystemWatcher::directoryChanged,
	    this,
	    &PwConnection::onRuntimeDirChanged
	);
}

void PwConnection::stopSocketWatcher() {
	if (this->socketWatcher == nullptr) return;

	this->socketWatcher->deleteLater();
	this->socketWatcher = nullptr;
}

void PwConnection::queueFatalError() {
	if (this->fatalErrorQueued) return;

	this->fatalErrorQueued = true;
	QMetaObject::invokeMethod(this, &PwConnection::onFatalError, Qt::QueuedConnection);
}

void PwConnection::onFatalError() {
	this->fatalErrorQueued = false;

	this->defaults.reset();
	this->registry.reset();
	this->core.shutdown();

	this->beginReconnect();
}

void PwConnection::onRuntimeDirChanged(const QString& /*path*/) {
	if (this->core.isValid()) {
		this->stopSocketWatcher();
		return;
	}

	this->tryConnect(true);
}

PwConnection* PwConnection::instance() {
	static PwConnection* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new PwConnection();
	}

	return instance;
}

} // namespace qs::service::pipewire
