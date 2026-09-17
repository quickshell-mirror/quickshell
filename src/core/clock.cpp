#include "clock.hpp"

#include <qdatetime.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#ifdef Q_OS_LINUX
#include <cerrno>
#include <cstdint>
#include <time.h> // NOLINT(modernize-deprecated-headers): POSIX clocks and itimerspec.

#include <qlogging.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

SystemClock::SystemClock(QObject* parent)
    : QObject(parent)
#ifdef Q_OS_LINUX
    // glibc defines these POSIX types/macros in private headers exported by time.h.
    // NOLINTNEXTLINE(misc-include-cleaner)
    , timerFd(timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC))
#endif
{
	QObject::connect(&this->timer, &QTimer::timeout, this, &SystemClock::onTimeout);
#ifdef Q_OS_LINUX
	// QTimer measures awake time. A wall-clock deadline must also expire while
	// suspended, so the first event loop iteration after resume sees fresh time.
	if (this->timerFd >= 0) {
		this->notifier.setSocket(this->timerFd);
		QObject::connect(
		    &this->notifier,
		    &QSocketNotifier::activated,
		    this,
		    &SystemClock::onRealtimeTimeout
		);
	} else {
		qWarning() << "SystemClock: could not create realtime timer, falling back to QTimer:" << errno;
	}
#endif
	this->update();
}

SystemClock::~SystemClock() {
#ifdef Q_OS_LINUX
	this->closeRealtimeTimer();
#endif
}

#ifdef Q_OS_LINUX
void SystemClock::closeRealtimeTimer() {
	this->notifier.setEnabled(false);
	if (this->timerFd >= 0) close(this->timerFd);
	this->timerFd = -1;
}

void SystemClock::onRealtimeTimeout() {
	uint64_t expirations = 0;
	ssize_t result = 0;
	do {
		result = read(this->timerFd, &expirations, sizeof(expirations));
	} while (result < 0 && errno == EINTR);

	if (result < 0 && errno == EAGAIN) return;
	if (result < 0 && errno != ECANCELED) {
		qWarning() << "SystemClock: could not read realtime timer, falling back to QTimer:" << errno;
		this->closeRealtimeTimer();
	}

	// ECANCELED means the system clock was set, possibly backwards. Resample
	// wall time instead of snapping to the old target, then arm a new deadline.
	this->update();
}
#endif

bool SystemClock::enabled() const { return this->mEnabled; }

void SystemClock::setEnabled(bool enabled) {
	if (enabled == this->mEnabled) return;
	this->mEnabled = enabled;
	emit this->enabledChanged();
	this->update();
}

SystemClock::Enum SystemClock::precision() const { return this->mPrecision; }

void SystemClock::setPrecision(SystemClock::Enum precision) {
	if (precision == this->mPrecision) return;
	this->mPrecision = precision;
	emit this->precisionChanged();
	this->update();
}

void SystemClock::onTimeout() {
	this->setTime(this->targetTime);
	this->schedule(this->targetTime);
}

void SystemClock::update() {
	if (this->mEnabled) {
		this->setTime(QDateTime::fromMSecsSinceEpoch(0));
		this->schedule(QDateTime::fromMSecsSinceEpoch(0));
	} else {
		this->timer.stop();
#ifdef Q_OS_LINUX
		if (this->timerFd >= 0) {
			this->notifier.setEnabled(false);
			const itimerspec disarmed {}; // NOLINT(misc-include-cleaner): exported by time.h.
			if (timerfd_settime(this->timerFd, 0, &disarmed, nullptr) < 0) {
				this->closeRealtimeTimer();
			}
		}
#endif
	}
}

void SystemClock::setTime(const QDateTime& targetTime) {
	auto currentTime = QDateTime::currentDateTime();
	auto offset = currentTime.msecsTo(targetTime);
	this->currentTime = offset > -500 && offset < 500 ? targetTime : currentTime;

	auto time = this->currentTime.time();
	this->currentTime.setTime(QTime(
	    this->mPrecision >= SystemClock::Hours ? time.hour() : 0,
	    this->mPrecision >= SystemClock::Minutes ? time.minute() : 0,
	    this->mPrecision >= SystemClock::Seconds ? time.second() : 0
	));

	emit this->dateChanged();
}

void SystemClock::schedule(const QDateTime& targetTime) {
	// A dateChanged handler may disable the clock while it is being updated.
	if (!this->mEnabled) return;

	auto secondPrecision = this->mPrecision >= SystemClock::Seconds;
	auto minutePrecision = this->mPrecision >= SystemClock::Minutes;
	auto hourPrecision = this->mPrecision >= SystemClock::Hours;

	auto currentTime = QDateTime::currentDateTime();

	auto offset = currentTime.msecsTo(targetTime);

	// timer skew
	auto nextTime = offset > 0 && offset < 500 ? targetTime : currentTime;

	auto baseTimeT = nextTime.time();
	nextTime.setTime(QTime(
	    hourPrecision ? baseTimeT.hour() : 0,
	    minutePrecision ? baseTimeT.minute() : 0,
	    secondPrecision ? baseTimeT.second() : 0
	));

	if (secondPrecision) nextTime = nextTime.addSecs(1);
	else if (minutePrecision) nextTime = nextTime.addSecs(60);
	else if (hourPrecision) nextTime = nextTime.addSecs(3600);

	this->targetTime = nextTime;
#ifdef Q_OS_LINUX
	if (this->timerFd >= 0) {
		auto deadline = nextTime.toMSecsSinceEpoch();
		itimerspec timeout {};
		timeout.it_value.tv_sec = deadline / 1000;
		timeout.it_value.tv_nsec = (deadline % 1000) * 1000000;
		if (timerfd_settime(
		        this->timerFd,
		        TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET,
		        &timeout,
		        nullptr
		    )
		    == 0)
		{
			this->notifier.setEnabled(true);
			return;
		}

		qWarning() << "SystemClock: could not arm realtime timer, falling back to QTimer:" << errno;
		this->closeRealtimeTimer();
	}
#endif

	auto delay = currentTime.msecsTo(nextTime);
	this->timer.start(static_cast<qint32>(delay));
}
