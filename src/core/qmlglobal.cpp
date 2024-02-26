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
