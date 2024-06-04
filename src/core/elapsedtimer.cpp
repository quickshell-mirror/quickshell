#include "elapsedtimer.hpp"

#include <qtypes.h>

ElapsedTimer::ElapsedTimer() { this->timer.start(); }

qreal ElapsedTimer::elapsed() { return static_cast<qreal>(this->elapsedNs()) / 1000000000.0; }

qreal ElapsedTimer::restart() { return static_cast<qreal>(this->restartNs()) / 1000000000.0; }

qint64 ElapsedTimer::elapsedMs() { return this->timer.elapsed(); }

qint64 ElapsedTimer::restartMs() { return this->timer.restart(); }

qint64 ElapsedTimer::elapsedNs() { return this->timer.nsecsElapsed(); }

qint64 ElapsedTimer::restartNs() {
	// see qelapsedtimer.cpp
	auto old = this->timer;
	this->timer.start();
	return old.durationTo(this->timer).count();
}
