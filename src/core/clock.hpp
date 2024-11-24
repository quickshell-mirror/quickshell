#pragma once

#include <qdatetime.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "util.hpp"

///! System clock accessor.
class SystemClock: public QObject {
	Q_OBJECT;
	/// If the clock should update. Defaults to true.
	///
	/// Setting enabled to false pauses the clock.
	Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged);
	/// The precision the clock should measure at. Defaults to `SystemClock.Seconds`.
	Q_PROPERTY(SystemClock::Enum precision READ precision WRITE setPrecision NOTIFY precisionChanged);
	/// The current hour.
	Q_PROPERTY(quint32 hours READ hours NOTIFY hoursChanged);
	/// The current minute, or 0 if @@precision is `SystemClock.Hours`.
	Q_PROPERTY(quint32 minutes READ minutes NOTIFY minutesChanged);
	/// The current second, or 0 if @@precision is `SystemClock.Hours` or `SystemClock.Minutes`.
	Q_PROPERTY(quint32 seconds READ seconds NOTIFY secondsChanged);
	QML_ELEMENT;

public:
	// must be named enum until docgen is ready to handle member enums better
	enum Enum : quint8 {
		Hours = 1,
		Minutes = 2,
		Seconds = 3,
	};
	Q_ENUM(Enum);

	explicit SystemClock(QObject* parent = nullptr);

	[[nodiscard]] bool enabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] SystemClock::Enum precision() const;
	void setPrecision(SystemClock::Enum precision);

signals:
	void enabledChanged();
	void precisionChanged();
	void hoursChanged();
	void minutesChanged();
	void secondsChanged();

private slots:
	void onTimeout();

private:
	bool mEnabled = true;
	SystemClock::Enum mPrecision = SystemClock::Seconds;
	quint32 mHours = 0;
	quint32 mMinutes = 0;
	quint32 mSeconds = 0;
	QTimer timer;
	QDateTime targetTime;

	void update();
	void setTime(const QDateTime& targetTime);
	void schedule(const QDateTime& targetTime);

	DECLARE_PRIVATE_MEMBER(SystemClock, hours, setHours, mHours, hoursChanged);
	DECLARE_PRIVATE_MEMBER(SystemClock, minutes, setMinutes, mMinutes, minutesChanged);
	DECLARE_PRIVATE_MEMBER(SystemClock, seconds, setSeconds, mSeconds, secondsChanged);
};
