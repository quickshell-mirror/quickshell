#pragma once

#include <qlogging.h>
#include <qloggingcategory.h>

namespace qs::log {
void initLogCategoryLevel(const char* name, QtMsgType defaultLevel = QtDebugMsg);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define QS_DECLARE_LOGGING_CATEGORY(name)                                                          \
	namespace qslogcat {                                                                             \
	Q_DECLARE_LOGGING_CATEGORY(name);                                                                \
	}                                                                                                \
	const QLoggingCategory& name()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define QS_LOGGING_CATEGORY(name, category, ...)                                                   \
	namespace qslogcat {                                                                             \
	Q_LOGGING_CATEGORY(name, category __VA_OPT__(, __VA_ARGS__));                                    \
	}                                                                                                \
	const QLoggingCategory& name() {                                                                 \
		static auto* init = []() {                                                                     \
			qs::log::initLogCategoryLevel(category __VA_OPT__(, __VA_ARGS__));                           \
			return &qslogcat::name;                                                                      \
		}();                                                                                           \
		return (init) ();                                                                              \
	}
