#pragma once

#include <utility>

#include <qdatetime.h>
#include <qfile.h>
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

class ThreadLogging: public QObject {
	Q_OBJECT;

public:
	explicit ThreadLogging(QObject* parent): QObject(parent) {}

	void init();
	void initFs();
	void setupFileLogging();

private slots:
	void onMessage(const LogMessage& msg);

private:
	QFile* file = nullptr;
	QTextStream fileStream;
};

class LoggingThreadProxy: public QObject {
	Q_OBJECT;

public:
	explicit LoggingThreadProxy() = default;

public slots:
	void initInThread();
	void initFs();

private:
	ThreadLogging* logging = nullptr;
};

class LogManager: public QObject {
	Q_OBJECT;

public:
	static void init();
	static void initFs();
	static LogManager* instance();

	static void formatMessage(QTextStream& stream, const LogMessage& msg, bool color, bool timestamp);

signals:
	void logMessage(LogMessage msg);

private:
	explicit LogManager();
	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	bool colorLogs;
	QTextStream stdoutStream;
	LoggingThreadProxy threadProxy;
};
