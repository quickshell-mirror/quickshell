#include "logging.hpp"
#include <cstdio>
#include <cstring>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <sys/mman.h>
#include <sys/sendfile.h>

#include "paths.hpp"

Q_LOGGING_CATEGORY(logLogging, "quickshell.logging", QtWarningMsg);

LogManager::LogManager()
    : colorLogs(qEnvironmentVariableIsEmpty("NO_COLOR"))
    , stdoutStream(stdout) {}

void LogManager::messageHandler(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& msg
) {
	auto message = LogMessage(type, context.category, msg.toUtf8());

	auto* self = LogManager::instance();

	LogManager::formatMessage(self->stdoutStream, message, self->colorLogs, false);
	self->stdoutStream << Qt::endl;

	emit self->logMessage(message);
}

LogManager* LogManager::instance() {
	static auto* instance = new LogManager(); // NOLINT
	return instance;
}

void LogManager::init() {
	auto* instance = LogManager::instance();

	qInstallMessageHandler(&LogManager::messageHandler);

	qCDebug(logLogging) << "Creating offthread logger...";
	auto* thread = new QThread();
	instance->threadProxy.moveToThread(thread);
	thread->start();
	QMetaObject::invokeMethod(&instance->threadProxy, "initInThread", Qt::BlockingQueuedConnection);
	qCDebug(logLogging) << "Logger initialized.";
}

void LogManager::initFs() {
	QMetaObject::invokeMethod(
	    &LogManager::instance()->threadProxy,
	    "initFs",
	    Qt::BlockingQueuedConnection
	);
}

void LogManager::formatMessage(
    QTextStream& stream,
    const LogMessage& msg,
    bool color,
    bool timestamp
) {
	if (timestamp) {
		if (color) stream << "\033[90m";
		stream << msg.time.toString("yyyy-MM-dd hh:mm:ss.zzz");
	}

	if (color) {
		switch (msg.type) {
		case QtDebugMsg: stream << "\033[34m DEBUG"; break;
		case QtInfoMsg: stream << "\033[32m  INFO"; break;
		case QtWarningMsg: stream << "\033[33m  WARN"; break;
		case QtCriticalMsg: stream << "\033[31m ERROR"; break;
		case QtFatalMsg: stream << "\033[31m FATAL"; break;
		}
	} else {
		switch (msg.type) {
		case QtDebugMsg: stream << " DEBUG"; break;
		case QtInfoMsg: stream << "  INFO"; break;
		case QtWarningMsg: stream << "  WARN"; break;
		case QtCriticalMsg: stream << " ERROR"; break;
		case QtFatalMsg: stream << " FATAL"; break;
		}
	}

	const auto isDefault = strcmp(msg.category, "default") == 0;

	if (color && !isDefault && msg.type != QtFatalMsg) stream << "\033[97m";

	if (!isDefault) {
		stream << ' ' << msg.category;
	}

	if (color && msg.type != QtFatalMsg) stream << "\033[0m";

	stream << ": " << msg.body;

	if (color && msg.type == QtFatalMsg) stream << "\033[0m";
}

void LoggingThreadProxy::initInThread() {
	this->logging = new ThreadLogging(this);
	this->logging->init();
}

void LoggingThreadProxy::initFs() { this->logging->initFs(); }

void ThreadLogging::init() {
	auto mfd = memfd_create("quickshell:logs", 0);

	if (mfd == -1) {
		qCCritical(logLogging) << "Failed to create memfd for initial log storage"
		                       << qt_error_string(-1);
		return;
	}

	this->file = new QFile();
	this->file->open(mfd, QFile::WriteOnly, QFile::AutoCloseHandle);
	this->fileStream.setDevice(this->file);

	// This connection is direct so it works while the event loop is destroyed between
	// QCoreApplication delete and Q(Gui)Application launch.
	QObject::connect(
	    LogManager::instance(),
	    &LogManager::logMessage,
	    this,
	    &ThreadLogging::onMessage,
	    Qt::DirectConnection
	);

	qCDebug(logLogging) << "Created memfd" << mfd << "for early logs.";
}

void ThreadLogging::initFs() {

	qCDebug(logLogging) << "Starting filesystem logging...";
	auto* runDir = QsPaths::instance()->instanceRunDir();

	if (!runDir) {
		qCCritical(logLogging
		) << "Could not start filesystem logging as the runtime directory could not be created.";
		return;
	}

	auto path = runDir->filePath("log.log");
	auto* file = new QFile(path);

	if (!file->open(QFile::WriteOnly | QFile::Truncate)) {
		qCCritical(logLogging
		) << "Could not start filesystem logger as the log file could not be created:"
		  << path;
		return;
	}

	qCDebug(logLogging) << "Copying memfd logs to log file...";

	auto* oldFile = this->file;
	oldFile->seek(0);
	sendfile(file->handle(), oldFile->handle(), nullptr, oldFile->size());
	this->file = file;
	this->fileStream.setDevice(file);
	delete oldFile;

	qCDebug(logLogging) << "Switched logging to disk logs.";

	auto* logManager = LogManager::instance();
	QObject::disconnect(logManager, &LogManager::logMessage, this, &ThreadLogging::onMessage);

	QObject::connect(
	    logManager,
	    &LogManager::logMessage,
	    this,
	    &ThreadLogging::onMessage,
	    Qt::QueuedConnection
	);

	qCDebug(logLogging) << "Switched threaded logger to queued eventloop connection.";
}

void ThreadLogging::onMessage(const LogMessage& msg) {
	if (this->fileStream.device() == nullptr) return;
	LogManager::formatMessage(this->fileStream, msg, false, true);
	this->fileStream << Qt::endl;
}
