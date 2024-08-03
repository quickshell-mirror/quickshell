#include "logging.hpp"
#include <cstdio>
#include <cstring>

#include <qlogging.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>

namespace {

bool COLOR_LOGS = false; // NOLINT

void formatMessage(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& msg,
    bool color
) {
	const auto* typeString = "[log error]";

	if (color) {
		switch (type) {
		case QtDebugMsg: typeString = "\033[34m DEBUG"; break;
		case QtInfoMsg: typeString = "\033[32m  INFO"; break;
		case QtWarningMsg: typeString = "\033[33m  WARN"; break;
		case QtCriticalMsg: typeString = "\033[31m ERROR"; break;
		case QtFatalMsg: typeString = "\033[31m FATAL"; break;
		}
	} else {
		switch (type) {
		case QtDebugMsg: typeString = " DEBUG"; break;
		case QtInfoMsg: typeString = "  INFO"; break;
		case QtWarningMsg: typeString = "  WARN"; break;
		case QtCriticalMsg: typeString = " ERROR"; break;
		case QtFatalMsg: typeString = " FATAL"; break;
		}
	}

	const auto isDefault = strcmp(context.category, "default") == 0;

	const char* format = nullptr;

	if (color) {
		if (type == QtFatalMsg) {
			if (isDefault) format = "%s: %s\033[0m\n";
			else format = "%s %s: %s\033[0m\n";
		} else {
			if (isDefault) format = "%s\033[0m: %s\n";
			else format = "%s \033[97m%s\033[0m: %s\n";
		}
	} else {
		if (isDefault) format = "%s: %s\n";
		else format = "%s %s: %s\n";
	}

	if (isDefault) {
		printf(format, typeString, msg.toStdString().c_str());
	} else {
		printf(format, typeString, context.category, msg.toStdString().c_str());
	}

	fflush(stdout);
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
	formatMessage(type, context, msg, COLOR_LOGS);
}

} // namespace

void LogManager::setup() {
	COLOR_LOGS = qEnvironmentVariableIsEmpty("NO_COLOR");
	qInstallMessageHandler(&messageHandler);
}
