#include "rootwrapper.hpp"
#include <cstdlib>
#include <utility>

#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qurl.h>

#include "scavenge.hpp"
#include "shell.hpp"
#include "watcher.hpp"

RootWrapper::RootWrapper(QString rootPath):
    QObject(nullptr), rootPath(std::move(rootPath)), engine(this) {
	this->reloadGraph(true);

	if (this->root == nullptr) {
		qCritical() << "could not create scene graph, exiting";
		exit(-1); // NOLINT
	}
}

QObject* RootWrapper::scavengeTargetFor(QObject* /* child */) { return this->root; }

void RootWrapper::reloadGraph(bool hard) {
	if (this->root != nullptr) {
		this->engine.clearComponentCache();
	}

	auto component = QQmlComponent(&this->engine, QUrl::fromLocalFile(this->rootPath));

	SCAVENGE_PARENT = hard ? nullptr : this;
	auto* obj = component.beginCreate(this->engine.rootContext());
	SCAVENGE_PARENT = nullptr;

	if (obj == nullptr) {
		qWarning() << component.errorString().toStdString().c_str();
		qWarning() << "failed to create root component";
		return;
	}

	auto* newRoot = qobject_cast<ShellRoot*>(obj);
	if (newRoot == nullptr) {
		qWarning() << "root component was not a QuickShell.ShellRoot";
		delete obj;
		return;
	}

	component.completeCreate();

	if (this->root != nullptr) {
		this->root->deleteLater();
		this->root = nullptr;
	}

	this->root = newRoot;
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
