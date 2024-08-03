#pragma once

#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qtextstream.h>
#include <qtmetamacros.h>

struct LogMessage {
	explicit LogMessage(QtMsgType type, const char* category, QByteArray body)
	    : type(type)
	    , category(category)
	    , body(std::move(body)) {}

	QtMsgType type;
	const char* category;
	QByteArray body;
};

class LogManager: public QObject {
	Q_OBJECT;

public:
	static LogManager* instance();

	static void formatMessage(QTextStream& stream, const LogMessage& msg, bool color);

signals:
	void logMessage(LogMessage msg);

private:
	explicit LogManager();
	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	bool colorLogs;
	QTextStream stdoutStream;
};
