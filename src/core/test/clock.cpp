#include "clock.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <time.h> // NOLINT(modernize-deprecated-headers): POSIX clock and tzset interfaces.

#include <qdatetime.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qregularexpression.h>
#include <qtenvironmentvariables.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtypes.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "../clock.hpp"

namespace {
std::atomic<qint64>& wallTimeMs() {
	static std::atomic<qint64> value = 0;
	return value;
}

qint64 timestamp(const char* value) {
	return QDateTime::fromString(QString::fromLatin1(value), Qt::ISODateWithMs).toMSecsSinceEpoch();
}
} // namespace

// Override wall time without changing the monotonic clock used by QTimer.
// NOLINTBEGIN(readability-identifier-naming, readability-inconsistent-declaration-parameter-name, misc-include-cleaner): POSIX and syscall interfaces.
extern "C" int clock_gettime(clockid_t id, timespec* value) noexcept {
	const auto now = wallTimeMs().load();
	if (now != 0 && (id == CLOCK_REALTIME || id == CLOCK_REALTIME_COARSE)) {
		value->tv_sec = now / 1000;
		value->tv_nsec = (now % 1000) * 1000000;
		return 0;
	}

	return static_cast<int>(syscall(SYS_clock_gettime, id, value));
}

extern "C" int gettimeofday(timeval* value, void* zone) noexcept {
	const auto now = wallTimeMs().load();
	if (now != 0) {
		value->tv_sec = now / 1000;
		value->tv_usec = (now % 1000) * 1000;
		return 0;
	}

	return static_cast<int>(syscall(SYS_gettimeofday, value, zone));
}

// NOLINTEND(readability-identifier-naming, readability-inconsistent-declaration-parameter-name, misc-include-cleaner)

void TestClock::init() {
	this->hadTimezone = qEnvironmentVariableIsSet("TZ");
	this->oldTimezone = qgetenv("TZ");
	QTest::failOnWarning(QRegularExpression(".*"));
}

void TestClock::cleanup() {
	wallTimeMs() = 0;
	if (this->hadTimezone) qputenv("TZ", this->oldTimezone);
	else qunsetenv("TZ");
	tzset();
}

void TestClock::timeZones_data() {
	QTest::addColumn<QByteArray>("zone");
	QTest::addColumn<qint64>("now");
	QTest::addColumn<int>("precision");
	QTest::addColumn<qint64>("expected");

	struct ClockCase {
		const char* name;
		const char* zone;
		const char* now;
		std::array<const char*, 3> expected;
	};

	const std::array cases {
	    ClockCase {
	        .name = "utc",
	        .zone = "UTC",
	        .now = "2026-10-25T01:30:12.550Z",
	        .expected =
	            {"2026-10-25T01:30:12.000Z", "2026-10-25T01:30:00.000Z", "2026-10-25T01:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "zurich-first-hour",
	        .zone = "Europe/Zurich",
	        .now = "2026-10-25T00:30:12.550Z",
	        .expected =
	            {"2026-10-25T00:30:12.000Z", "2026-10-25T00:30:00.000Z", "2026-10-25T00:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "zurich-second-hour",
	        .zone = "Europe/Zurich",
	        .now = "2026-10-25T01:30:12.550Z",
	        .expected =
	            {"2026-10-25T01:30:12.000Z", "2026-10-25T01:30:00.000Z", "2026-10-25T01:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "zurich-before-fall-back",
	        .zone = "Europe/Zurich",
	        .now = "2026-10-25T00:59:59.550Z",
	        .expected =
	            {"2026-10-25T00:59:59.000Z", "2026-10-25T00:59:00.000Z", "2026-10-25T00:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "zurich-after-fall-back",
	        .zone = "Europe/Zurich",
	        .now = "2026-10-25T02:00:12.550Z",
	        .expected =
	            {"2026-10-25T02:00:12.000Z", "2026-10-25T02:00:00.000Z", "2026-10-25T02:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "zurich-before-spring-forward",
	        .zone = "Europe/Zurich",
	        .now = "2027-03-28T00:59:12.550Z",
	        .expected =
	            {"2027-03-28T00:59:12.000Z", "2027-03-28T00:59:00.000Z", "2027-03-28T00:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "zurich-after-spring-forward",
	        .zone = "Europe/Zurich",
	        .now = "2027-03-28T01:30:12.550Z",
	        .expected =
	            {"2027-03-28T01:30:12.000Z", "2027-03-28T01:30:00.000Z", "2027-03-28T01:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "half-hour-zone",
	        .zone = "Asia/Kolkata",
	        .now = "2026-10-25T04:45:12.550Z",
	        .expected =
	            {"2026-10-25T04:45:12.000Z", "2026-10-25T04:45:00.000Z", "2026-10-25T04:30:00.000Z"},
	    },
	    ClockCase {
	        .name = "quarter-hour-zone",
	        .zone = "Asia/Kathmandu",
	        .now = "2026-10-25T04:45:12.550Z",
	        .expected =
	            {"2026-10-25T04:45:12.000Z", "2026-10-25T04:45:00.000Z", "2026-10-25T04:15:00.000Z"},
	    },
	    ClockCase {
	        .name = "lord-howe-first-hour",
	        .zone = "Australia/Lord_Howe",
	        .now = "2026-04-04T14:45:12.550Z",
	        .expected =
	            {"2026-04-04T14:45:12.000Z", "2026-04-04T14:45:00.000Z", "2026-04-04T14:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "lord-howe-second-hour",
	        .zone = "Australia/Lord_Howe",
	        .now = "2026-04-04T15:15:12.550Z",
	        .expected =
	            {"2026-04-04T15:15:12.000Z", "2026-04-04T15:15:00.000Z", "2026-04-04T14:00:00.000Z"},
	    },
	    ClockCase {
	        .name = "lord-howe-spring-forward",
	        .zone = "Australia/Lord_Howe",
	        .now = "2026-10-03T15:45:12.550Z",
	        .expected =
	            {"2026-10-03T15:45:12.000Z", "2026-10-03T15:45:00.000Z", "2026-10-03T15:30:00.000Z"},
	    },
	};
	const std::array precisions {SystemClock::Seconds, SystemClock::Minutes, SystemClock::Hours};
	for (const auto& entry: cases) {
		for (std::size_t i = 0; i < precisions.size(); i++) {
			const auto name = QByteArray(entry.name) + '-' + QByteArray::number(precisions.at(i));
			QTest::newRow(name.constData())
			    << QByteArray(entry.zone) << timestamp(entry.now) << static_cast<int>(precisions.at(i))
			    << timestamp(entry.expected.at(i));
		}
	}
}

void TestClock::timeZones() {
	QFETCH(const QByteArray, zone);
	QFETCH(const qint64, now);
	QFETCH(const int, precision);
	QFETCH(const qint64, expected);
	qputenv("TZ", zone);
	tzset();
	wallTimeMs() = now;
	SystemClock clock;
	clock.setPrecision(static_cast<SystemClock::Enum>(precision));
	QCOMPARE(clock.date().toMSecsSinceEpoch(), expected);
}

void TestClock::earlyTickAcrossFallBack() {
	qputenv("TZ", "Europe/Zurich");
	tzset();
	wallTimeMs() = timestamp("2026-10-25T00:59:59.550Z");
	SystemClock clock;
	wallTimeMs() = timestamp("2026-10-25T00:59:59.750Z");
	QVERIFY(QMetaObject::invokeMethod(&clock, "onTimeout", Qt::DirectConnection));
	QCOMPARE(clock.date().toMSecsSinceEpoch(), timestamp("2026-10-25T01:00:00.000Z"));
}

QTEST_GUILESS_MAIN(TestClock);
