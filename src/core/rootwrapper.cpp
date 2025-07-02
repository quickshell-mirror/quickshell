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

#include "../ui/reload_popup.hpp"
#include "../window/floatingwindow.hpp"
#include "generation.hpp"
#include "instanceinfo.hpp"
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
		qInfo() << "Reloading configuration...";
		QuickshellSettings::reset();
	}

	QDir::setCurrent(this->originalWorkingDirectory);

	auto url = QUrl::fromLocalFile(this->rootPath);
	// unless the original file comes from the qsintercept scheme
	url.setScheme("qsintercept");
	auto component = QQmlComponent(generation->engine, url);

	if (!component.isReady()) {
		qCritical() << "Failed to load configuration";
		QString errorString = "Failed to load configuration";

		auto errors = component.errors();
		for (auto& error: errors) {
			auto rel = "**/" % rootPath.relativeFilePath(error.url().path());
			auto msg = "  caused by " % rel % '[' % QString::number(error.line()) % ':'
			         % QString::number(error.column()) % "]: " % error.description();
			errorString += '\n' % msg;
			qCritical().noquote() << msg;
		}

		auto newFiles = generation->scanner.scannedFiles;
		generation->destroy();

		if (this->generation != nullptr) {
			if (this->generation->setExtraWatchedFiles(newFiles)) {
				qInfo() << "Watching additional files picked up in reload for changes...";
			}

			auto showPopup = true;
			if (this->generation->qsgInstance != nullptr) {
				this->generation->qsgInstance->clearReloadPopupInhibit();
				emit this->generation->qsgInstance->reloadFailed(errorString);
				showPopup = !this->generation->qsgInstance->isReloadPopupInhibited();
			}

			if (showPopup)
				qs::ui::ReloadPopup::spawnPopup(InstanceInfo::CURRENT.instanceId, true, errorString);
		}

		if (this->generation != nullptr && this->generation->qsgInstance != nullptr) {
			emit this->generation->qsgInstance->reloadFailed(errorString);
		}

		return;
	}

	auto* newRoot = component.beginCreate(generation->engine->rootContext());

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

	if (isReload) {
		auto showPopup = true;

		if (this->generation->qsgInstance != nullptr) {
			this->generation->qsgInstance->clearReloadPopupInhibit();
			emit this->generation->qsgInstance->reloadCompleted();
			showPopup = !this->generation->qsgInstance->isReloadPopupInhibited();
		}

		if (showPopup) qs::ui::ReloadPopup::spawnPopup(InstanceInfo::CURRENT.instanceId, false, "");
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
