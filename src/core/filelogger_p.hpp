#pragma once

#include <qfile.h>
#include <qobject.h>
#include <qtextstream.h>
#include <qtmetamacros.h>

#include "logging.hpp"

class FileLogger: public QObject {
	Q_OBJECT;

public:
	explicit FileLogger() = default;

public slots:
	void init();

private slots:
	void onMessage(const LogMessage& msg);

private:
	QTextStream fileStream;
};
