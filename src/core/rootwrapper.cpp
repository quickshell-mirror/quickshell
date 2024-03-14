#include "rootwrapper.hpp"
#include <cstdlib>
#include <utility>

#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qurl.h>

#include "generation.hpp"
#include "qmlglobal.hpp"
#include "shell.hpp"
#include "watcher.hpp"

RootWrapper::RootWrapper(QString rootPath)
    : QObject(nullptr)
    , rootPath(std::move(rootPath))
    , originalWorkingDirectory(QDir::current().absolutePath()) {
	// clang-format off
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::watchFilesChanged, this, &RootWrapper::onWatchFilesChanged);
	// clang-format on

	this->reloadGraph(true);

	if (this->generation == nullptr) {
		qCritical() << "could not create scene graph, exiting";
		exit(-1); // NOLINT
	}
}

RootWrapper::~RootWrapper() {
	// event loop may no longer be running so deleteLater is not an option
	if (this->generation != nullptr) {
		delete this->generation->root;
		this->generation->root = nullptr;
	}

	delete this->generation;
}

void RootWrapper::reloadGraph(bool hard) {
	auto* generation = new EngineGeneration();

	// todo: move into EngineGeneration
	if (this->generation != nullptr) {
		QuickshellSettings::reset();
	}

	QDir::setCurrent(this->originalWorkingDirectory);

	auto component = QQmlComponent(&generation->engine, QUrl::fromLocalFile(this->rootPath));

	auto* obj = component.beginCreate(generation->engine.rootContext());

	if (obj == nullptr) {
		qWarning() << component.errorString().toStdString().c_str();
		qWarning() << "failed to create root component";
		delete generation;
		return;
	}

	auto* newRoot = qobject_cast<ShellRoot*>(obj);
	if (newRoot == nullptr) {
		qWarning() << "root component was not a Quickshell.ShellRoot";
		delete obj;
		delete generation;
		return;
	}

	generation->root = newRoot;

	component.completeCreate();

	generation->onReload(hard ? nullptr : this->generation);
	if (hard) delete this->generation;
	this->generation = generation;

	qInfo() << "Configuration Loaded";

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
