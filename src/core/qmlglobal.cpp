#include "qmlglobal.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdir.h>
#include <qguiapplication.h>
#include <qjsengine.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <unistd.h>

#include "qmlscreen.hpp"
#include "rootwrapper.hpp"

QuickshellSettings* QuickshellSettings::instance() {
	static QuickshellSettings* instance = nullptr; // NOLINT
	if (instance == nullptr) {
		QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
		instance = new QuickshellSettings();
	}
	return instance;
}

void QuickshellSettings::reset() { QuickshellSettings::instance()->mWatchFiles = true; }

QString QuickshellSettings::workingDirectory() const { // NOLINT
	return QDir::current().absolutePath();
}

void QuickshellSettings::setWorkingDirectory(QString workingDirectory) { // NOLINT
	QDir::setCurrent(workingDirectory);
	emit this->workingDirectoryChanged();
}

bool QuickshellSettings::watchFiles() const { return this->mWatchFiles; }

void QuickshellSettings::setWatchFiles(bool watchFiles) {
	if (watchFiles == this->mWatchFiles) return;
	this->mWatchFiles = watchFiles;
	emit this->watchFilesChanged();
}

QuickshellGlobal::QuickshellGlobal(QObject* parent): QObject(parent) {
	// clang-format off
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::workingDirectoryChanged, this, &QuickshellGlobal::workingDirectoryChanged);
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::watchFilesChanged, this, &QuickshellGlobal::watchFilesChanged);
	// clang-format on

	auto* app = QCoreApplication::instance();
	auto* guiApp = qobject_cast<QGuiApplication*>(app);

	if (guiApp != nullptr) {
		// clang-format off
		QObject::connect(guiApp, &QGuiApplication::primaryScreenChanged, this, &QuickshellGlobal::updateScreens);
		QObject::connect(guiApp, &QGuiApplication::screenAdded, this, &QuickshellGlobal::updateScreens);
		QObject::connect(guiApp, &QGuiApplication::screenRemoved, this, &QuickshellGlobal::updateScreens);
		// clang-format on

		this->updateScreens();
	}
}

qint32 QuickshellGlobal::processId() const { // NOLINT
	return getpid();
}

qsizetype QuickshellGlobal::screensCount(QQmlListProperty<QuickshellScreenInfo>* prop) {
	return static_cast<QuickshellGlobal*>(prop->object)->mScreens.size(); // NOLINT
}

QuickshellScreenInfo*
QuickshellGlobal::screenAt(QQmlListProperty<QuickshellScreenInfo>* prop, qsizetype i) {
	return static_cast<QuickshellGlobal*>(prop->object)->mScreens.at(i); // NOLINT
}

QQmlListProperty<QuickshellScreenInfo> QuickshellGlobal::screens() {
	return QQmlListProperty<QuickshellScreenInfo>(
	    this,
	    nullptr,
	    &QuickshellGlobal::screensCount,
	    &QuickshellGlobal::screenAt
	);
}

void QuickshellGlobal::reload(bool hard) {
	auto* rootobj = QQmlEngine::contextForObject(this)->engine()->parent();
	auto* root = qobject_cast<RootWrapper*>(rootobj);

	if (root == nullptr) {
		qWarning() << "cannot find RootWrapper for reload, ignoring request";
		return;
	}

	root->reloadGraph(hard);
}

QString QuickshellGlobal::workingDirectory() const { // NOLINT
	return QuickshellSettings::instance()->workingDirectory();
}

void QuickshellGlobal::setWorkingDirectory(QString workingDirectory) { // NOLINT
	QuickshellSettings::instance()->setWorkingDirectory(std::move(workingDirectory));
}

bool QuickshellGlobal::watchFiles() const { // NOLINT
	return QuickshellSettings::instance()->watchFiles();
}

void QuickshellGlobal::setWatchFiles(bool watchFiles) { // NOLINT
	QuickshellSettings::instance()->setWatchFiles(watchFiles);
}

void QuickshellGlobal::updateScreens() {
	auto screens = QGuiApplication::screens();
	auto newScreens = QList<QuickshellScreenInfo*>();

	for (auto* newScreen: screens) {
		for (auto i = 0; i < this->mScreens.length(); i++) {
			auto* oldScreen = this->mScreens[i];
			if (newScreen == oldScreen->screen) {
				newScreens.push_back(oldScreen);
				this->mScreens.remove(i);
				goto next;
			}
		}

		{
			auto* si = new QuickshellScreenInfo(this, newScreen);
			QQmlEngine::setObjectOwnership(si, QQmlEngine::CppOwnership);
			newScreens.push_back(si);
		}
	next:;
	}

	for (auto* oldScreen: this->mScreens) {
		oldScreen->deleteLater();
	}

	this->mScreens = newScreens;
	emit this->screensChanged();
}

QVariant QuickshellGlobal::env(const QString& variable) { // NOLINT
	auto vstr = variable.toStdString();
	if (!qEnvironmentVariableIsSet(vstr.data())) return QVariant::fromValue(nullptr);

	return qEnvironmentVariable(vstr.data());
}
