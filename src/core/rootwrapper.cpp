#include "rootwrapper.hpp"
#include <cstdlib>
#include <utility>

#include <qcoreapplication.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qtimer.h>
#include <qurl.h>

#include "plugin.hpp"
#include "qmlglobal.hpp"
#include "reload.hpp"
#include "shell.hpp"
#include "singleton.hpp"
#include "watcher.hpp"

RootWrapper::RootWrapper(QString rootPath)
    : QObject(nullptr)
    , rootPath(std::move(rootPath))
    , originalWorkingDirectory(QDir::current().absolutePath()) {
	// clang-format off
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::watchFilesChanged, this, &RootWrapper::onWatchFilesChanged);
	// clang-format on

	this->reloadGraph(true);

	if (this->root == nullptr) {
		qCritical() << "could not create scene graph, exiting";
		exit(-1); // NOLINT
	}
}

RootWrapper::~RootWrapper() {
	// event loop may no longer be running so deleteLater is not an option
	delete this->root;
}

void RootWrapper::reloadGraph(bool hard) {
	auto* oldEngine = this->engine;
	this->engine = new QQmlEngine(this);

	auto* app = QCoreApplication::instance();
	QObject::connect(this->engine, &QQmlEngine::quit, app, &QCoreApplication::quit);
	QObject::connect(this->engine, &QQmlEngine::exit, app, &QCoreApplication::exit);

	if (this->root != nullptr) {
		QuickshellSettings::reset();
		SingletonRegistry::instance()->flip();
	}

	QDir::setCurrent(this->originalWorkingDirectory);

	auto component = QQmlComponent(this->engine, QUrl::fromLocalFile(this->rootPath));

	auto* obj = component.beginCreate(this->engine->rootContext());

	if (obj == nullptr) {
		qWarning() << component.errorString().toStdString().c_str();
		qWarning() << "failed to create root component";
		return;
	}

	auto* newRoot = qobject_cast<ShellRoot*>(obj);
	if (newRoot == nullptr) {
		qWarning() << "root component was not a Quickshell.ShellRoot";
		delete obj;
		return;
	}

	component.completeCreate();

	auto* oldRoot = this->root;
	this->root = newRoot;

	this->root->onReload(hard ? nullptr : oldRoot);

	if (oldRoot != nullptr) {
		oldRoot->deleteLater();

		QTimer::singleShot(0, [this, newRoot]() {
			if (this->root == newRoot) {
				QuickshellPlugin::runOnReload();
				PostReloadHook::postReloadTree(this->root);
			}
		});
	} else {
		PostReloadHook::postReloadTree(newRoot);
		QuickshellPlugin::runOnReload();
	}

	delete oldEngine;

	this->onWatchFilesChanged();
}

void RootWrapper::onWatchFilesChanged() {
	auto watchFiles = QuickshellSettings::instance()->watchFiles();

	if (watchFiles && this->configWatcher == nullptr) {
		this->configWatcher = new FiletreeWatcher();
		this->configWatcher->addPath(QFileInfo(this->rootPath).dir().path());

		QObject::connect(
		    this->configWatcher,
		    &FiletreeWatcher::fileChanged,
		    this,
		    &RootWrapper::onWatchedFilesChanged
		);
	} else if (!watchFiles && this->configWatcher != nullptr) {
		this->configWatcher->deleteLater();
		this->configWatcher = nullptr;
	}
}

void RootWrapper::onWatchedFilesChanged() { this->reloadGraph(false); }
