#include "qmlglobal.hpp"

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

#include "qmlscreen.hpp"
#include "rootwrapper.hpp"

QuickshellGlobal::QuickshellGlobal(QObject* parent): QObject(parent) {
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

void QuickshellGlobal::updateScreens() {
	auto screens = QGuiApplication::screens();
	this->mScreens.resize(screens.size());

	for (auto i = 0; i < screens.size(); i++) {
		if (this->mScreens[i] != nullptr) {
			this->mScreens[i]->screen = nullptr;
			this->mScreens[i]->setParent(nullptr); // delete if not owned by the js engine
		}

		this->mScreens[i] = new QuickshellScreenInfo(this, screens[i]);
	}

	emit this->screensChanged();
}

QVariant QuickshellGlobal::env(const QString& variable) { // NOLINT
	auto vstr = variable.toStdString();
	if (!qEnvironmentVariableIsSet(vstr.data())) return QVariant::fromValue(nullptr);

	return qEnvironmentVariable(vstr.data());
}

QString QuickshellGlobal::workingDirectory() const { // NOLINT
	return QDir::current().absolutePath();
}

void QuickshellGlobal::setWorkingDirectory(const QString& workingDirectory) { // NOLINT
	QDir::setCurrent(workingDirectory);
	emit this->workingDirectoryChanged();
}

static QuickshellGlobal* g_instance = nullptr; // NOLINT

QuickshellGlobal* QuickshellGlobal::create(QQmlEngine* /*unused*/, QJSEngine* /*unused*/) {
	return QuickshellGlobal::instance();
}

QuickshellGlobal* QuickshellGlobal::instance() {
	if (g_instance == nullptr) g_instance = new QuickshellGlobal();
	QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
	return g_instance;
}

void QuickshellGlobal::deleteInstance() {
	delete g_instance;
	g_instance = nullptr;
}
