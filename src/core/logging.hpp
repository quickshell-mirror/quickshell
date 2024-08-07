#pragma once

#include <utility>

#include <qdatetime.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtextstream.h>
#include <qtmetamacros.h>

struct LogMessage {
	explicit LogMessage(
	    QtMsgType type,
	    const char* category,
	    QByteArray body,
	    QDateTime time = QDateTime::currentDateTime()
	)
	    : type(type)
	    , time(std::move(time))
	    , category(category)
	    , body(std::move(body)) {}

	QtMsgType type;
	QDateTime time;
	const char* category;
	QByteArray body;
};

class LogManager: public QObject {
	Q_OBJECT;

public:
	static LogManager* instance();

	static void formatMessage(QTextStream& stream, const LogMessage& msg, bool color, bool timestamp);

signals:
	void logMessage(LogMessage msg);

private:
	explicit LogManager();
	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	bool colorLogs;
	QTextStream stdoutStream;
};
