#include "qmlglobal.hpp"

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlscreen.hpp"
#include "rootwrapper.hpp"

QtShellGlobal::QtShellGlobal(QObject* parent): QObject(parent) {
	auto* app = QCoreApplication::instance();
	auto* guiApp = qobject_cast<QGuiApplication*>(app);

	if (guiApp != nullptr) {
		// clang-format off
		QObject::connect(guiApp, &QGuiApplication::primaryScreenChanged, this, &QtShellGlobal::updateScreens);
		QObject::connect(guiApp, &QGuiApplication::screenAdded, this, &QtShellGlobal::updateScreens);
		QObject::connect(guiApp, &QGuiApplication::screenRemoved, this, &QtShellGlobal::updateScreens);
		// clang-format on

		this->updateScreens();
	}
}

qsizetype QtShellGlobal::screensCount(QQmlListProperty<QtShellScreenInfo>* prop) {
	return static_cast<QtShellGlobal*>(prop->object)->mScreens.size(); // NOLINT
}

QtShellScreenInfo* QtShellGlobal::screenAt(QQmlListProperty<QtShellScreenInfo>* prop, qsizetype i) {
	return static_cast<QtShellGlobal*>(prop->object)->mScreens.at(i); // NOLINT
}

QQmlListProperty<QtShellScreenInfo> QtShellGlobal::screens() {
	return QQmlListProperty<QtShellScreenInfo>(
	    this,
	    nullptr,
	    &QtShellGlobal::screensCount,
	    &QtShellGlobal::screenAt
	);
}

void QtShellGlobal::reload(bool hard) {
	auto* rootobj = QQmlEngine::contextForObject(this)->engine()->parent();
	auto* root = qobject_cast<RootWrapper*>(rootobj);

	if (root == nullptr) {
		qWarning() << "cannot find RootWrapper for reload, ignoring request";
		return;
	}

	root->reloadGraph(hard);
}

void QtShellGlobal::updateScreens() {
	auto screens = QGuiApplication::screens();
	this->mScreens.resize(screens.size());

	for (auto i = 0; i < screens.size(); i++) {
		if (this->mScreens[i] != nullptr) {
			this->mScreens[i]->screen = nullptr;
			this->mScreens[i]->setParent(nullptr); // delete if not owned by the js engine
		}

		this->mScreens[i] = new QtShellScreenInfo(this, screens[i]);
	}

	emit this->screensChanged();
}
