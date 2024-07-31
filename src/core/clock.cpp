#include "clock.hpp"

#include <qdatetime.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>

#include "util.hpp"

SystemClock::SystemClock(QObject* parent): QObject(parent) {
	QObject::connect(&this->timer, &QTimer::timeout, this, &SystemClock::update);
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

void SystemClock::update() {
	auto time = QTime::currentTime();

	if (this->mEnabled) {
		auto secondPrecision = this->mPrecision >= SystemClock::Seconds;
		auto secondChanged = this->setSeconds(secondPrecision ? time.second() : 0);

		auto minutePrecision = this->mPrecision >= SystemClock::Minutes;
		auto minuteChanged = this->setMinutes(minutePrecision ? time.minute() : 0);

		auto hourPrecision = this->mPrecision >= SystemClock::Hours;
		auto hourChanged = this->setHours(hourPrecision ? time.hour() : 0);

		DropEmitter::call(secondChanged, minuteChanged, hourChanged);

		auto nextTime = QTime(
		    hourPrecision ? time.hour() : 0,
		    minutePrecision ? time.minute() : 0,
		    secondPrecision ? time.second() : 0
		);

		if (secondPrecision) nextTime = nextTime.addSecs(1);
		else if (minutePrecision) nextTime = nextTime.addSecs(60);
		else if (hourPrecision) nextTime = nextTime.addSecs(3600);

		auto delay = time.msecsTo(nextTime);
		// day rollover
		if (delay < 0) delay += 86400000;

		this->timer.start(delay);
	} else {
		this->timer.stop();
	}
}

DEFINE_MEMBER_GETSET(SystemClock, hours, setHours);
DEFINE_MEMBER_GETSET(SystemClock, minutes, setMinutes);
DEFINE_MEMBER_GETSET(SystemClock, seconds, setSeconds);
