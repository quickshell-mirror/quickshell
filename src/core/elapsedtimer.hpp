#pragma once

#include <qelapsedtimer.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

///! Measures time between events
/// The ElapsedTimer measures time since its last restart, and is useful
/// for determining the time between events that don't supply it.
class ElapsedTimer: public QObject {
	Q_OBJECT;
	QML_ELEMENT;

public:
	explicit ElapsedTimer();

	/// Return the number of seconds since the timer was last
	/// started or restarted, with nanosecond precision.
	Q_INVOKABLE qreal elapsed();

	/// Restart the timer, returning the number of seconds since
	/// the timer was last started or restarted, with nanosecond precision.
	Q_INVOKABLE qreal restart();

	/// Return the number of milliseconds since the timer was last
	/// started or restarted.
	Q_INVOKABLE qint64 elapsedMs();

	/// Restart the timer, returning the number of milliseconds since
	/// the timer was last started or restarted.
	Q_INVOKABLE qint64 restartMs();

	/// Return the number of nanoseconds since the timer was last
	/// started or restarted.
	Q_INVOKABLE qint64 elapsedNs();

	/// Restart the timer, returning the number of nanoseconds since
	/// the timer was last started or restarted.
	Q_INVOKABLE qint64 restartNs();

private:
	QElapsedTimer timer;
};
