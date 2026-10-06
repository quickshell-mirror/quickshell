#include "clock.hpp"
#include <atomic>
#include <cerrno>
#include <time.h> // NOLINT(modernize-deprecated-headers): POSIX clocks and itimerspec.

#include <qdatetime.h>
#include <qhash.h>
#include <qobject.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtestsupport_core.h>
#include <qtypes.h>
#include <sys/syscall.h>
#include <sys/timerfd.h> // IWYU pragma: keep
#include <sys/types.h>
#include <unistd.h>

#include "../clock.hpp"

namespace {
// Model wall time advancing without monotonic time (suspend), and wall-clock
// corrections, without requiring CAP_SYS_TIME or suspending the test machine.
std::atomic<qint64>& wallOffsetMs() {
	static std::atomic<qint64> offset = 0;
	return offset;
}

struct Timer {
	qint64 deadline = 0;
	int flags = 0;
	bool canceled = false;
};

QHash<int, Timer>& timers() {
	static QHash<int, Timer> state;
	return state;
}
} // namespace

// Exported to Qt as well as this executable. Only realtime is offset; Qt's
// monotonic event loop timers continue normally.
// These names/signatures are required by libc and the linker --wrap interface.
// POSIX clock declarations are exported by time.h from private glibc headers.
// NOLINTBEGIN(bugprone-reserved-identifier, readability-identifier-naming, readability-inconsistent-declaration-parameter-name, misc-include-cleaner)
extern "C" int clock_gettime(clockid_t id, timespec* value) noexcept {
	auto result = static_cast<int>(syscall(SYS_clock_gettime, id, value));
	if (result == 0 && (id == CLOCK_REALTIME || id == CLOCK_REALTIME_COARSE)) {
		auto offset = wallOffsetMs().load();
		value->tv_sec += offset / 1000;
		value->tv_nsec += (offset % 1000) * 1000000;
		if (value->tv_nsec < 0) {
			value->tv_sec--;
			value->tv_nsec += 1000000000;
		} else if (value->tv_nsec >= 1000000000) {
			value->tv_sec++;
			value->tv_nsec -= 1000000000;
		}
	}
	return result;
}

extern "C" {
int __real_timerfd_create(int clockid, int flags);
int __real_timerfd_settime(int fd, int flags, const itimerspec* value, itimerspec* old);
ssize_t __real_read(int fd, void* buffer, size_t size);
int __real_close(int fd);

int __wrap_timerfd_create(int clockid, int flags) {
	auto fd = __real_timerfd_create(clockid, flags);
	if (fd >= 0 && clockid == CLOCK_REALTIME) timers().insert(fd, Timer {});
	return fd;
}

int __wrap_timerfd_settime(int fd, int flags, const itimerspec* value, itimerspec* old) {
	auto entry = timers().find(fd);
	if (entry == timers().end()) return __real_timerfd_settime(fd, flags, value, old);
	auto& timer = entry.value();
	timer.deadline = value->it_value.tv_sec * 1000 + value->it_value.tv_nsec / 1000000;
	timer.flags = flags;
	timer.canceled = false;
	auto adjusted = *value;
	if (timer.deadline != 0 && (flags & TFD_TIMER_ABSTIME) != 0) {
		auto realDeadline = timer.deadline - wallOffsetMs().load();
		adjusted.it_value.tv_sec = realDeadline / 1000;
		adjusted.it_value.tv_nsec = (realDeadline % 1000) * 1000000;
	}
	return __real_timerfd_settime(fd, flags, &adjusted, old);
}

ssize_t __wrap_read(int fd, void* buffer, size_t size) {
	auto result = __real_read(fd, buffer, size);
	auto entry = timers().find(fd);
	if (entry != timers().end() && entry->canceled) {
		entry->canceled = false;
		errno = ECANCELED;
		return -1;
	}
	return result;
}

int __wrap_close(int fd) {
	timers().remove(fd);
	return __real_close(fd);
}
}

// NOLINTEND(bugprone-reserved-identifier, readability-identifier-naming, readability-inconsistent-declaration-parameter-name, misc-include-cleaner)

namespace {
void advanceWallTime(qint64 milliseconds, bool discontinuous = false) {
	wallOffsetMs() += milliseconds;
	auto now = QDateTime::currentMSecsSinceEpoch();
	for (auto it = timers().begin(); it != timers().end(); ++it) {
		if (it->deadline == 0) continue;
		auto absolute = (it->flags & TFD_TIMER_ABSTIME) != 0;
		auto canceled = discontinuous && absolute && (it->flags & TFD_TIMER_CANCEL_ON_SET) != 0;
		if (canceled || (absolute && it->deadline <= now)) {
			it->canceled = canceled;
			itimerspec expired {};
			expired.it_value.tv_nsec = 1;
			__real_timerfd_settime(it.key(), 0, &expired, nullptr);
		}
	}
}

QDateTime expectedDate(SystemClock::Enum precision) {
	auto now = QDateTime::currentDateTime();
	auto time = now.time();
	now.setTime(QTime(
	    time.hour(),
	    precision >= SystemClock::Minutes ? time.minute() : 0,
	    precision >= SystemClock::Seconds ? time.second() : 0
	));
	return now;
}
} // namespace

void TestClock::init() {
	QVERIFY(timers().isEmpty());
	wallOffsetMs() = 0;
	// Keep the next minute/hour boundary far away so a QTimer cannot pass the
	// resume tests accidentally by expiring during their 500 ms wait.
	auto now = QDateTime::currentDateTime();
	auto aligned = now;
	aligned.setTime(QTime(now.time().hour(), 15, 10));
	wallOffsetMs() = now.msecsTo(aligned);
}

void TestClock::resume_data() {
	QTest::addColumn<int>("precision");
	QTest::newRow("seconds") << static_cast<int>(SystemClock::Seconds);
	QTest::newRow("minutes") << static_cast<int>(SystemClock::Minutes);
	QTest::newRow("hours") << static_cast<int>(SystemClock::Hours);
}

void TestClock::resume() {
	QFETCH(const int, precision);
	SystemClock clock;
	clock.setPrecision(static_cast<SystemClock::Enum>(precision));
	const QSignalSpy spy(&clock, &SystemClock::dateChanged);
	advanceWallTime(14 * 3600000 + 14 * 60000 + 48000);
	QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 500);
	QCOMPARE(clock.date(), expectedDate(clock.precision()));
}

void TestClock::clockSetBackwards() {
	SystemClock clock;
	clock.setPrecision(SystemClock::Minutes);
	const QSignalSpy spy(&clock, &SystemClock::dateChanged);
	advanceWallTime(-3600000, true);
	QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 500);
	QCOMPARE(clock.date(), expectedDate(clock.precision()));
}

void TestClock::disabled() {
	SystemClock clock;
	clock.setPrecision(SystemClock::Minutes);
	clock.setEnabled(false);
	auto previous = clock.date();
	QSignalSpy spy(&clock, &SystemClock::dateChanged);
	advanceWallTime(3600000);
	QTest::qWait(50);
	QVERIFY(spy.isEmpty());
	QCOMPARE(clock.date(), previous);
	clock.setEnabled(true);
	QCOMPARE(clock.date(), expectedDate(clock.precision()));
	spy.clear();
	advanceWallTime(60000);
	QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 500);
	QCOMPARE(clock.date(), expectedDate(clock.precision()));
}

void TestClock::changePrecision() {
	SystemClock clock;
	clock.setPrecision(SystemClock::Hours);
	clock.setPrecision(SystemClock::Minutes);
	const QSignalSpy spy(&clock, &SystemClock::dateChanged);
	advanceWallTime(60000);
	QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 500);
	QCOMPARE(clock.date(), expectedDate(SystemClock::Minutes));
}

void TestClock::disableFromNotification() {
	SystemClock clock;
	clock.setPrecision(SystemClock::Minutes);
	QObject::connect(&clock, &SystemClock::dateChanged, &clock, [&clock]() {
		clock.setEnabled(false);
	});
	advanceWallTime(60000);
	QTRY_VERIFY_WITH_TIMEOUT(!clock.enabled(), 500);
	const QSignalSpy spy(&clock, &SystemClock::dateChanged);
	advanceWallTime(60000);
	QTest::qWait(50);
	QVERIFY(spy.isEmpty());
}

void TestClock::successiveDeadlines() {
	SystemClock clock;
	clock.setPrecision(SystemClock::Minutes);
	QSignalSpy spy(&clock, &SystemClock::dateChanged);
	for (auto i = 0; i < 3; i++) {
		spy.clear();
		advanceWallTime(60000);
		QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 500);
		QCOMPARE(clock.date(), expectedDate(SystemClock::Minutes));
	}
}

void TestClock::normalTick() {
	const SystemClock clock;
	const QSignalSpy spy(&clock, &SystemClock::dateChanged);
	QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 1500);
	QCOMPARE(clock.date(), expectedDate(SystemClock::Seconds));
}

QTEST_GUILESS_MAIN(TestClock);
