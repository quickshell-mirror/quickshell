#include "clock.hpp"

#include <qdatetime.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>

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
	this->setTime(this->targetTime);
	this->schedule(this->targetTime);
}

void SystemClock::update() {
	if (this->mEnabled) {
		this->setTime(QDateTime::fromMSecsSinceEpoch(0));
		this->schedule(QDateTime::fromMSecsSinceEpoch(0));
	} else {
		this->timer.stop();
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

	auto delay = currentTime.msecsTo(nextTime);

	this->timer.start(static_cast<qint32>(delay));
	this->targetTime = nextTime;
}
