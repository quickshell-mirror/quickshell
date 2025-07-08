#pragma once

// The logging rule parser from qloggingregistry_p.h and qloggingregistry.cpp.

// Was unable to properly link the functions when directly using the headers (which we depend
// on anyway), so below is a slightly stripped down copy. Making the originals link would
// be preferable.

#include <qflags.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstringview.h>
#include <qtypes.h>

#include "logcat.hpp"

namespace qs::log {
QS_DECLARE_LOGGING_CATEGORY(logLogging);

namespace qt_logging_registry {

class QLoggingRule {
public:
	QLoggingRule();
	QLoggingRule(QStringView pattern, bool enabled);
	[[nodiscard]] int pass(QLatin1StringView categoryName, QtMsgType type) const;

	enum PatternFlag : quint8 {
		FullText = 0x1,
		LeftFilter = 0x2,
		RightFilter = 0x4,
		MidFilter = LeftFilter | RightFilter
	};
	Q_DECLARE_FLAGS(PatternFlags, PatternFlag)

	QString category;
	int messageType;
	PatternFlags flags;
	bool enabled;

private:
	void parse(QStringView pattern);
};

} // namespace qt_logging_registry

} // namespace qs::log
