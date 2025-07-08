// The logging rule parser from qloggingregistry_p.h and qloggingregistry.cpp.

// Was unable to properly link the functions when directly using the headers (which we depend
// on anyway), so below is a slightly stripped down copy. Making the originals link would
// be preferable.

#include <utility>

#include <qbytearrayview.h>
#include <qchar.h>
#include <qflags.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstringtokenizer.h>
#include <qstringview.h>
#include <qtypes.h>

#include "logcat.hpp"
#include "logging_qtprivate.hpp"

namespace qs::log {
QS_DECLARE_LOGGING_CATEGORY(logLogging);

namespace qt_logging_registry {

class QLoggingSettingsParser {
public:
	void setContent(QStringView content);

	[[nodiscard]] QList<QLoggingRule> rules() const { return this->mRules; }

private:
	void parseNextLine(QStringView line);

private:
	QList<QLoggingRule> mRules;
};

void QLoggingSettingsParser::setContent(QStringView content) {
	this->mRules.clear();
	for (auto line: qTokenize(content, u';')) this->parseNextLine(line);
}

void QLoggingSettingsParser::parseNextLine(QStringView line) {
	// Remove whitespace at start and end of line:
	line = line.trimmed();

	const qsizetype equalPos = line.indexOf(u'=');
	if (equalPos != -1) {
		if (line.lastIndexOf(u'=') == equalPos) {
			const auto key = line.left(equalPos).trimmed();
			const QStringView pattern = key;
			const auto valueStr = line.mid(equalPos + 1).trimmed();
			int value = -1;
			if (valueStr == QString("true")) value = 1;
			else if (valueStr == QString("false")) value = 0;
			QLoggingRule rule(pattern, (value == 1));
			if (rule.flags != 0 && (value != -1)) this->mRules.append(std::move(rule));
			else
				qCWarning(logLogging, "Ignoring malformed logging rule: '%s'", line.toUtf8().constData());
		} else {
			qCWarning(logLogging, "Ignoring malformed logging rule: '%s'", line.toUtf8().constData());
		}
	}
}

QLoggingRule::QLoggingRule(QStringView pattern, bool enabled): messageType(-1), enabled(enabled) {
	this->parse(pattern);
}

void QLoggingRule::parse(QStringView pattern) {
	QStringView p;

	// strip trailing ".messagetype"
	if (pattern.endsWith(QString(".debug"))) {
		p = pattern.chopped(6); // strlen(".debug")
		this->messageType = QtDebugMsg;
	} else if (pattern.endsWith(QString(".info"))) {
		p = pattern.chopped(5); // strlen(".info")
		this->messageType = QtInfoMsg;
	} else if (pattern.endsWith(QString(".warning"))) {
		p = pattern.chopped(8); // strlen(".warning")
		this->messageType = QtWarningMsg;
	} else if (pattern.endsWith(QString(".critical"))) {
		p = pattern.chopped(9); // strlen(".critical")
		this->messageType = QtCriticalMsg;
	} else {
		p = pattern;
	}

	const QChar asterisk = u'*';
	if (!p.contains(asterisk)) {
		this->flags = FullText;
	} else {
		if (p.endsWith(asterisk)) {
			this->flags |= LeftFilter;
			p = p.chopped(1);
		}
		if (p.startsWith(asterisk)) {
			this->flags |= RightFilter;
			p = p.mid(1);
		}
		if (p.contains(asterisk)) // '*' only supported at start/end
			this->flags = PatternFlags();
	}

	this->category = p.toString();
}

int QLoggingRule::pass(QLatin1StringView cat, QtMsgType msgType) const {
	// check message type
	if (this->messageType > -1 && this->messageType != msgType) return 0;

	if (this->flags == FullText) {
		// full match
		if (this->category == cat) return (this->enabled ? 1 : -1);
		else return 0;
	}

	const qsizetype idx = cat.indexOf(this->category);
	if (idx >= 0) {
		if (this->flags == MidFilter) {
			// matches somewhere
			return (this->enabled ? 1 : -1);
		} else if (this->flags == LeftFilter) {
			// matches left
			if (idx == 0) return (this->enabled ? 1 : -1);
		} else if (this->flags == RightFilter) {
			// matches right
			if (idx == (cat.size() - this->category.size())) return (this->enabled ? 1 : -1);
		}
	}
	return 0;
}

} // namespace qt_logging_registry

} // namespace qs::log
