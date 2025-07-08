#pragma once

#include <qobject.h>
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

class DelayedQmlIncubationController: public QQmlIncubationController {
	// Do nothing.
	// This ensures lazy loaders don't start blocking before onReload creates windows.
};
