#include "interface.hpp"
#include <cstring>
#include <utility>

#include <qapplication.h>
#include <qboxlayout.h>
#include <qconfig.h>
#include <qdesktopservices.h>
#include <qfont.h>
#include <qfontinfo.h>
#include <qlabel.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qtversion.h>
#include <qwidget.h>

#include "build.hpp"

class ReportLabel: public QWidget {
public:
	ReportLabel(const QString& label, const QString& content, QWidget* parent): QWidget(parent) {
		auto* layout = new QHBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(new QLabel(label, this));

		auto* cl = new QLabel(content, this);
		cl->setTextInteractionFlags(Qt::TextSelectableByMouse);
		layout->addWidget(cl);

		layout->addStretch();
	}
};

CrashReporterGui::CrashReporterGui(QString reportFolder, int pid)
    : reportFolder(std::move(reportFolder)) {
	this->setWindowFlags(Qt::Window);

	auto textHeight = QFontInfo(QFont()).pixelSize();

	auto* mainLayout = new QVBoxLayout(this);

	auto qtVersionMatches = strcmp(qVersion(), QT_VERSION_STR) == 0;
	if (qtVersionMatches) {
		mainLayout->addWidget(new QLabel(
		    "<u>Quickshell has crashed. Please submit a bug report to help us fix it.</u>",
		    this
		));
	} else {
		mainLayout->addWidget(
		    new QLabel("<u>Quickshell has crashed, likely due to a Qt version mismatch.</u>", this)
		);
	}

	mainLayout->addSpacing(textHeight);

	mainLayout->addWidget(new QLabel("General information", this));
	mainLayout->addWidget(new ReportLabel("Git Revision:", GIT_REVISION, this));
	mainLayout->addWidget(new QLabel(
	    QString::fromLatin1("Runtime Qt version: ") % qVersion() % ", Buildtime Qt version: "
	        % QT_VERSION_STR,
	    this
	));
	mainLayout->addWidget(new ReportLabel("Crashed process ID:", QString::number(pid), this));
	mainLayout->addWidget(new ReportLabel("Crash report folder:", this->reportFolder, this));
	mainLayout->addSpacing(textHeight);

	if (qtVersionMatches) {
		mainLayout->addWidget(new QLabel("Please open a bug report for this issue via github or email.")
		);
	} else {
		mainLayout->addWidget(new QLabel(
		    "Please rebuild Quickshell against the current Qt version.\n"
		    "If this does not solve the problem, please open a bug report via github or email."
		));
	}

	mainLayout->addWidget(new ReportLabel(
	    "Github:",
	    "https://github.com/quickshell-mirror/quickshell/issues/new?template=crash.yml",
	    this
	));

	mainLayout->addWidget(new ReportLabel("Email:", "quickshell-bugs@outfoxxed.me", this));

	auto* buttons = new QWidget(this);
	buttons->setMinimumWidth(900);
	auto* buttonLayout = new QHBoxLayout(buttons);
	buttonLayout->setContentsMargins(0, 0, 0, 0);

	auto* reportButton = new QPushButton("Open report page", buttons);
	reportButton->setDefault(true);
	QObject::connect(reportButton, &QPushButton::clicked, this, &CrashReporterGui::openReportUrl);
	buttonLayout->addWidget(reportButton);

	auto* openFolderButton = new QPushButton("Open crash folder", buttons);
	QObject::connect(openFolderButton, &QPushButton::clicked, this, &CrashReporterGui::openFolder);
	buttonLayout->addWidget(openFolderButton);

	auto* cancelButton = new QPushButton("Exit", buttons);
	QObject::connect(cancelButton, &QPushButton::clicked, this, &CrashReporterGui::cancel);
	buttonLayout->addWidget(cancelButton);

	mainLayout->addWidget(buttons);

	mainLayout->addStretch();
	this->setFixedSize(this->sizeHint());
}

void CrashReporterGui::openFolder() {
	QDesktopServices::openUrl(QUrl::fromLocalFile(this->reportFolder));
}

void CrashReporterGui::openReportUrl() {
	QDesktopServices::openUrl(
	    QUrl("https://github.com/outfoxxed/quickshell/issues/new?template=crash.yml")
	);
}

void CrashReporterGui::cancel() { QApplication::quit(); }
