#pragma once

#include <qwidget.h>

class CrashReporterGui: public QWidget {
public:
	CrashReporterGui(QString reportFolder, int pid);

private slots:
	void openFolder();

	static void openReportUrl();
	static void cancel();

private:
	QString reportFolder;
};
