#pragma once

#include <utility>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qhash.h>
#include <qlatin1stringview.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "logcat.hpp"

QS_DECLARE_LOGGING_CATEGORY(logBare);

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
	quint16 readCategoryId = 0;

	static void formatMessage(
	    QTextStream& stream,
	    const LogMessage& msg,
	    bool color,
	    bool timestamp,
	    const QString& prefix = ""
	);
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

namespace qt_logging_registry {
class QLoggingRule;
}

struct CategoryFilter {
	explicit CategoryFilter() = default;
	explicit CategoryFilter(QLoggingCategory* category)
	    : debug(category->isDebugEnabled())
	    , info(category->isInfoEnabled())
	    , warn(category->isWarningEnabled())
	    , critical(category->isCriticalEnabled()) {}

	[[nodiscard]] bool shouldDisplay(QtMsgType type) const;
	void apply(QLoggingCategory* category) const;
	void applyRule(QLatin1StringView category, const qt_logging_registry::QLoggingRule& rule);

	bool debug = true;
	bool info = true;
	bool warn = true;
	bool critical = true;
};

class LogManager: public QObject {
	Q_OBJECT;

public:
	static void init(
	    bool color,
	    bool timestamp,
	    bool sparseOnly,
	    QtMsgType defaultLevel,
	    const QString& rules,
	    const QString& prefix = ""
	);

	static void initFs();
	static LogManager* instance();

	bool colorLogs = true;
	bool timestampLogs = false;

	[[nodiscard]] QString rulesString() const;
	[[nodiscard]] QtMsgType defaultLevel() const;
	[[nodiscard]] bool isSparse() const;

	[[nodiscard]] CategoryFilter getFilter(QLatin1StringView category);

signals:
	void logMessage(LogMessage msg, bool showInSparse);

private:
	explicit LogManager();
	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	static void filterCategory(QLoggingCategory* category);

	QLoggingCategory::CategoryFilter lastCategoryFilter = nullptr;
	bool sparse = false;
	QString prefix;
	QString mRulesString;
	QList<qt_logging_registry::QLoggingRule>* rules = nullptr;
	QtMsgType mDefaultLevel = QtWarningMsg;
	QHash<QLatin1StringView, QtMsgType> defaultLevels;
	QHash<const void*, CategoryFilter> sparseFilters;
	QHash<QLatin1StringView, CategoryFilter> allFilters;

	QTextStream stdoutStream;
	LoggingThreadProxy threadProxy;

	friend void initLogCategoryLevel(const char* name, QtMsgType defaultLevel);
};

bool readEncodedLogs(
    QFile* file,
    const QString& path,
    bool timestamps,
    int tail,
    bool follow,
    const QString& rulespec
);

} // namespace qs::log

using LogManager = qs::log::LogManager;
