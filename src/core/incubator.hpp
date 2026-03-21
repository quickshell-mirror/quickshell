#pragma once

#include <qobject.h>
#include <qpointer.h>
#include <qqmlincubator.h>
#include <qtmetamacros.h>

#include "logcat.hpp"

QS_DECLARE_LOGGING_CATEGORY(logIncubator);

class QsQmlIncubator
    : public QObject
    , public QQmlIncubator {
	Q_OBJECT;

public:
	explicit QsQmlIncubator(QsQmlIncubator::IncubationMode mode, QObject* parent = nullptr)
	    : QObject(parent)
	    , QQmlIncubator(mode) {}

	void statusChanged(QQmlIncubator::Status status) override;

signals:
	void completed();
	void failed();
};

class QSGRenderLoop;

class QsIncubationController
    : public QObject
    , public QQmlIncubationController {
	Q_OBJECT

public:
	void initLoop();
	void setIncubationMode(bool render);
	void incubateLater();

protected:
	void timerEvent(QTimerEvent* event) override;

public slots:
	void incubate();
	void animationStopped();
	void updateIncubationTime();

protected:
	void incubatingObjectCountChanged(int count) override;

private:
// QPointer did not work with forward declarations prior to 6.7
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	QPointer<QSGRenderLoop> renderLoop = nullptr;
#else
	QSGRenderLoop* renderLoop = nullptr;
#endif
	int incubationTime = 0;
	int timerId = 0;
	bool followRenderloop = false;
};
