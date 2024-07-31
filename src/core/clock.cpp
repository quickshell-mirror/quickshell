#include "clock.hpp"

#include <qdatetime.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>

#include "util.hpp"

SystemClock::SystemClock(QObject* parent): QObject(parent) {
	QObject::connect(&this->timer, &QTimer::timeout, this, &SystemClock::onTimeout);
	this->update();
}

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
	this->setTime(this->nextTime);
	this->schedule(this->nextTime);
}

void SystemClock::update() {
	if (this->mEnabled) {
		this->setTime(QTime::currentTime());
		this->schedule(QTime::currentTime());
	} else {
		this->timer.stop();
	}
}

void SystemClock::setTime(QTime time) {
	auto secondPrecision = this->mPrecision >= SystemClock::Seconds;
	auto secondChanged = this->setSeconds(secondPrecision ? time.second() : 0);

	auto minutePrecision = this->mPrecision >= SystemClock::Minutes;
	auto minuteChanged = this->setMinutes(minutePrecision ? time.minute() : 0);

	auto hourPrecision = this->mPrecision >= SystemClock::Hours;
	auto hourChanged = this->setHours(hourPrecision ? time.hour() : 0);

	DropEmitter::call(secondChanged, minuteChanged, hourChanged);
}

void SystemClock::schedule(QTime floor) {
	auto secondPrecision = this->mPrecision >= SystemClock::Seconds;
	auto minutePrecision = this->mPrecision >= SystemClock::Minutes;
	auto hourPrecision = this->mPrecision >= SystemClock::Hours;

setnext:
	auto nextTime = QTime(
	    hourPrecision ? floor.hour() : 0,
	    minutePrecision ? floor.minute() : 0,
	    secondPrecision ? floor.second() : 0
	);

	if (secondPrecision) nextTime = nextTime.addSecs(1);
	else if (minutePrecision) nextTime = nextTime.addSecs(60);
	else if (hourPrecision) nextTime = nextTime.addSecs(3600);

	auto delay = QTime::currentTime().msecsTo(nextTime);

	// If off by more than 2 hours we likely wrapped around midnight.
	if (delay < -7200000) delay += 86400000;
	else if (delay < 0) {
		// Otherwise its just the timer being unstable.
		floor = QTime::currentTime();
		goto setnext;
	}

	this->timer.start(delay);
	this->nextTime = nextTime;
}

DEFINE_MEMBER_GETSET(SystemClock, hours, setHours);
DEFINE_MEMBER_GETSET(SystemClock, minutes, setMinutes);
DEFINE_MEMBER_GETSET(SystemClock, seconds, setSeconds);
