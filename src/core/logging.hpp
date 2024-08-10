#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qhash.h>
#include <qlatin1stringview.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::log {

struct LogMessage {
	explicit LogMessage() = default;

	explicit LogMessage(
	    QtMsgType type,
	    QLatin1StringView category,
	    QByteArray body,
	    QDateTime time = QDateTime::currentDateTime()
	)
	    : type(type)
	    , time(std::move(time))
	    , category(category)
	    , body(std::move(body)) {}

	bool operator==(const LogMessage& other) const;

	QtMsgType type = QtDebugMsg;
	QDateTime time;
	QLatin1StringView category;
	QByteArray body;

	static void formatMessage(QTextStream& stream, const LogMessage& msg, bool color, bool timestamp);
};

size_t qHash(const LogMessage& message);

class ThreadLogging;

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

struct CategoryFilter {
	explicit CategoryFilter() = default;
	explicit CategoryFilter(QLoggingCategory* category)
	    : debug(category->isDebugEnabled())
	    , info(category->isInfoEnabled())
	    , warn(category->isWarningEnabled())
	    , critical(category->isCriticalEnabled()) {}

	bool debug = true;
	bool info = true;
	bool warn = true;
	bool critical = true;
};

class LogManager: public QObject {
	Q_OBJECT;

public:
	static void init(bool color, bool sparseOnly);
	static void initFs();
	static LogManager* instance();

	bool colorLogs = true;

signals:
	void logMessage(LogMessage msg, bool showInSparse);

private:
	explicit LogManager();
	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	static void filterCategory(QLoggingCategory* category);

	QLoggingCategory::CategoryFilter lastCategoryFilter = nullptr;
	QHash<const void*, CategoryFilter> sparseFilters;

	QTextStream stdoutStream;
	LoggingThreadProxy threadProxy;
};

bool readEncodedLogs(QIODevice* device);

} // namespace qs::log

using LogManager = qs::log::LogManager;
