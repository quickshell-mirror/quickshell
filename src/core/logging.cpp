#include "logging.hpp"
#include <cstdio>
#include <cstring>

#include <qlogging.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <qtmetamacros.h>

LogManager::LogManager(): colorLogs(qEnvironmentVariableIsEmpty("NO_COLOR")), stdoutStream(stdout) {
	qInstallMessageHandler(&LogManager::messageHandler);
}

void LogManager::messageHandler(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& msg
) {
	auto message = LogMessage(type, context.category, msg.toUtf8());

	auto* self = LogManager::instance();

	LogManager::formatMessage(self->stdoutStream, message, self->colorLogs);
	self->stdoutStream << Qt::endl;

	emit self->logMessage(message);
}

LogManager* LogManager::instance() {
	static auto* instance = new LogManager(); // NOLINT
	return instance;
}

void LogManager::formatMessage(QTextStream& stream, const LogMessage& msg, bool color) {
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
}
