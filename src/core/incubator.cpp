#include "incubator.hpp"

#include <qlogging.h>
#include <qqmlincubator.h>
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
