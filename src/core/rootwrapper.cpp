#include "rootwrapper.hpp"
#include <cstdlib>
#include <utility>

#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qquickitem.h>
#include <qtmetamacros.h>
#include <qurl.h>

#include "../window/floatingwindow.hpp"
#include "generation.hpp"
#include "qmlglobal.hpp"
#include "scan.hpp"

RootWrapper::RootWrapper(QString rootPath, QString shellId)
    : QObject(nullptr)
    , rootPath(std::move(rootPath))
    , shellId(std::move(shellId))
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
		this->generation->shutdown();
	}
}

void RootWrapper::reloadGraph(bool hard) {
	auto rootPath = QFileInfo(this->rootPath).dir();
	auto scanner = QmlScanner(rootPath);
	scanner.scanQmlFile(this->rootPath);

	auto* generation = new EngineGeneration(rootPath, std::move(scanner));
	generation->wrapper = this;

	// todo: move into EngineGeneration
	if (this->generation != nullptr) {
		QuickshellSettings::reset();
	}

	QDir::setCurrent(this->originalWorkingDirectory);

	auto url = QUrl::fromLocalFile(this->rootPath);
	// unless the original file comes from the qsintercept scheme
	url.setScheme("qsintercept");
	auto component = QQmlComponent(generation->engine, url);

	auto* newRoot = component.beginCreate(generation->engine->rootContext());

	if (newRoot == nullptr) {
		const QString error = "failed to create root component\n" + component.errorString();
		qWarning().noquote() << error;
		generation->destroy();

		if (this->generation != nullptr && this->generation->qsgInstance != nullptr) {
			emit this->generation->qsgInstance->reloadFailed(error);
		}

		return;
	}

	if (auto* item = qobject_cast<QQuickItem*>(newRoot)) {
		auto* window = new FloatingWindowInterface();
		item->setParent(window);
		item->setParentItem(window->contentItem());
		window->setWidth(static_cast<int>(item->width()));
		window->setHeight(static_cast<int>(item->height()));
		newRoot = window;
	}

	generation->root = newRoot;

	component.completeCreate();

	if (this->generation) {
		QObject::disconnect(this->generation, nullptr, this, nullptr);
	}

	auto isReload = this->generation != nullptr;
	generation->onReload(hard ? nullptr : this->generation);

	if (hard && this->generation) {
		this->generation->destroy();
	}

	this->generation = generation;

	qInfo() << "Configuration Loaded";

	QObject::connect(this->generation, &QObject::destroyed, this, &RootWrapper::generationDestroyed);
	QObject::connect(
	    this->generation,
	    &EngineGeneration::filesChanged,
	    this,
	    &RootWrapper::onWatchedFilesChanged
	);

	this->onWatchFilesChanged();

	if (isReload && this->generation->qsgInstance != nullptr) {
		emit this->generation->qsgInstance->reloadCompleted();
	}
}

void RootWrapper::generationDestroyed() { this->generation = nullptr; }

void RootWrapper::onWatchFilesChanged() {
	auto watchFiles = QuickshellSettings::instance()->watchFiles();
	if (this->generation != nullptr) {
		this->generation->setWatchingFiles(watchFiles);
	}
}

void RootWrapper::onWatchedFilesChanged() { this->reloadGraph(false); }
