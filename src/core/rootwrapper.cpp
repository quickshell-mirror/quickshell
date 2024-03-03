#include "rootwrapper.hpp"
#include <cstdlib>
#include <utility>

#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qtimer.h>
#include <qurl.h>

#include "reload.hpp"
#include "shell.hpp"
#include "watcher.hpp"

RootWrapper::RootWrapper(QString rootPath)
    : QObject(nullptr)
    , rootPath(std::move(rootPath))
    , engine(this) {
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
	if (this->root != nullptr) {
		this->engine.clearComponentCache();
	}

	auto component = QQmlComponent(&this->engine, QUrl::fromLocalFile(this->rootPath));

	auto* obj = component.beginCreate(this->engine.rootContext());

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
			if (this->root == newRoot) PostReloadHook::postReloadTree(this->root);
		});
	} else {
		PostReloadHook::postReloadTree(newRoot);
	}

	this->onConfigChanged();
}

void RootWrapper::onConfigChanged() {
	auto config = this->root->config();

	if (config.mWatchFiles && this->configWatcher == nullptr) {
		this->configWatcher = new FiletreeWatcher();
		this->configWatcher->addPath(QFileInfo(this->rootPath).dir().path());

		QObject::connect(this->root, &ShellRoot::configChanged, this, &RootWrapper::onConfigChanged);

		QObject::connect(
		    this->configWatcher,
		    &FiletreeWatcher::fileChanged,
		    this,
		    &RootWrapper::onWatchedFilesChanged
		);
	} else if (!config.mWatchFiles && this->configWatcher != nullptr) {
		this->configWatcher->deleteLater();
		this->configWatcher = nullptr;
	}
}

void RootWrapper::onWatchedFilesChanged() { this->reloadGraph(false); }
