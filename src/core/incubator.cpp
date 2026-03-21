#include "incubator.hpp"

#include <private/qsgrenderloop_p.h>
#include <qabstractanimation.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qminmax.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlincubator.h>
#include <qscreen.h>
#include <qtmetamacros.h>

#include "logcat.hpp"

QS_LOGGING_CATEGORY(logIncubator, "quickshell.incubator", QtWarningMsg);

void QsQmlIncubator::statusChanged(QQmlIncubator::Status status) {
	switch (status) {
	case QQmlIncubator::Ready: emit this->completed(); break;
	case QQmlIncubator::Error: emit this->failed(); break;
	default: break;
	}
}

void QsIncubationController::initLoop() {
	auto* app = static_cast<QGuiApplication*>(QGuiApplication::instance()); // NOLINT
	this->renderLoop = QSGRenderLoop::instance();

	QObject::connect(
	    app,
	    &QGuiApplication::screenAdded,
	    this,
	    &QsIncubationController::updateIncubationTime
	);

	QObject::connect(
	    app,
	    &QGuiApplication::screenRemoved,
	    this,
	    &QsIncubationController::updateIncubationTime
	);

	this->updateIncubationTime();

	QObject::connect(
	    this->renderLoop,
	    &QSGRenderLoop::timeToIncubate,
	    this,
	    &QsIncubationController::incubate
	);

	QAnimationDriver* animationDriver = this->renderLoop->animationDriver();
	if (animationDriver) {
		QObject::connect(
		    animationDriver,
		    &QAnimationDriver::stopped,
		    this,
		    &QsIncubationController::animationStopped
		);
	} else {
		qCInfo(logIncubator) << "Render loop does not have animation driver, animationStopped cannot "
		                        "be used to trigger incubation.";
	}
}

void QsIncubationController::setIncubationMode(bool render) {
	if (render == this->followRenderloop) return;
	this->followRenderloop = render;

	if (render) {
		qCDebug(logIncubator) << "Incubation mode changed: render loop driven";
	} else {
		qCDebug(logIncubator) << "Incubation mode changed: event loop driven";
	}

	if (!render && this->incubatingObjectCount()) this->incubateLater();
}

void QsIncubationController::timerEvent(QTimerEvent* /*event*/) {
	this->killTimer(this->timerId);
	this->timerId = 0;
	this->incubate();
}

void QsIncubationController::incubateLater() {
	if (this->followRenderloop) {
		if (this->timerId != 0) {
			this->killTimer(this->timerId);
			this->timerId = 0;
		}

		// Incubate again at the end of the event processing queue
		QMetaObject::invokeMethod(this, &QsIncubationController::incubate, Qt::QueuedConnection);
	} else if (this->timerId == 0) {
		// Wait for a while before processing the next batch. Using a
		// timer to avoid starvation of system events.
		this->timerId = this->startTimer(this->incubationTime);
	}
}

void QsIncubationController::incubate() {
	if ((!this->followRenderloop || this->renderLoop) && this->incubatingObjectCount()) {
		if (!this->followRenderloop) {
			this->incubateFor(10);
			if (this->incubatingObjectCount()) this->incubateLater();
		} else if (this->renderLoop->interleaveIncubation()) {
			this->incubateFor(this->incubationTime);
		} else {
			this->incubateFor(this->incubationTime * 2);
			if (this->incubatingObjectCount()) this->incubateLater();
		}
	}
}

void QsIncubationController::animationStopped() { this->incubate(); }

void QsIncubationController::incubatingObjectCountChanged(int count) {
	if (count
	    && (!this->followRenderloop
	        || (this->renderLoop && !this->renderLoop->interleaveIncubation())))
	{
		this->incubateLater();
	}
}

void QsIncubationController::updateIncubationTime() {
	auto* screen = QGuiApplication::primaryScreen();
	if (!screen) return;

	// 1/3 frame on primary screen
	this->incubationTime = qMax(1, static_cast<int>(1000 / screen->refreshRate() / 3));
}
