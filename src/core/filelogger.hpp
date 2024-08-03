#pragma once

#include <qthread.h>
#include <qtmetamacros.h>
class FileLoggerThread: public QThread {
	Q_OBJECT;

public:
	static void init();

private:
	explicit FileLoggerThread() = default;
};
