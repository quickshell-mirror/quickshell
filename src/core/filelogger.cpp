#include "filelogger.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qtextstream.h>

#include "filelogger_p.hpp"
#include "logging.hpp"
#include "paths.hpp"

Q_LOGGING_CATEGORY(logLogger, "quickshell.logger", QtWarningMsg);

void FileLoggerThread::init() {
	auto* thread = new FileLoggerThread();
	auto* logger = new FileLogger();
	logger->moveToThread(thread);
	thread->start();
	QMetaObject::invokeMethod(logger, "init", Qt::BlockingQueuedConnection);
}

void FileLogger::init() {
	qCDebug(logLogger) << "Initializing filesystem logger...";
	auto* runDir = QsPaths::instance()->instanceRunDir();

	if (!runDir) {
		qCCritical(logLogger
		) << "Could not start filesystem logger as the runtime directory could not be created.";
		return;
	}

	auto path = runDir->filePath("log.log");
	auto* file = new QFile(path);

	if (!file->open(QFile::WriteOnly | QFile::Truncate)) {
		qCCritical(logLogger
		) << "Could not start filesystem logger as the log file could not be created:"
		  << path;
		return;
	}

	this->fileStream.setDevice(file);

	QObject::connect(
	    LogManager::instance(),
	    &LogManager::logMessage,
	    this,
	    &FileLogger::onMessage,
	    Qt::QueuedConnection
	);

	qDebug(logLogger) << "Initialized filesystem logger";
}

void FileLogger::onMessage(const LogMessage& msg) {
	LogManager::formatMessage(this->fileStream, msg, false, true);
	this->fileStream << Qt::endl;
}
