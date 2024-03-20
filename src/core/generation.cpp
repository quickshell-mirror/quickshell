#include "generation.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlincubator.h>
#include <qtimer.h>

#include "incubator.hpp"
#include "plugin.hpp"
#include "qsintercept.hpp"
#include "reload.hpp"
#include "scan.hpp"

static QHash<QQmlEngine*, EngineGeneration*> g_generations; // NOLINT

EngineGeneration::EngineGeneration(QmlScanner scanner)
    : scanner(std::move(scanner))
    , interceptNetFactory(this->scanner.qmldirIntercepts) {
	g_generations.insert(&this->engine, this);

	this->engine.addUrlInterceptor(&this->urlInterceptor);
	this->engine.setNetworkAccessManagerFactory(&this->interceptNetFactory);
	this->engine.setIncubationController(&this->delayedIncubationController);
}

EngineGeneration::~EngineGeneration() {
	g_generations.remove(&this->engine);
	if (this->root != nullptr) this->root->deleteLater();
}

void EngineGeneration::onReload(EngineGeneration* old) {
	if (old != nullptr) {
		// if the old generation holds the window incubation controller as the
		// new generation acquires it then incubators will hang intermittently
		old->incubationControllers.clear();
		old->engine.setIncubationController(&old->delayedIncubationController);
	}

	auto* app = QCoreApplication::instance();
	QObject::connect(&this->engine, &QQmlEngine::quit, app, &QCoreApplication::quit);
	QObject::connect(&this->engine, &QQmlEngine::exit, app, &QCoreApplication::exit);

	this->root->onReload(old == nullptr ? nullptr : old->root);
	this->singletonRegistry.onReload(old == nullptr ? nullptr : &old->singletonRegistry);

	delete old;

	if (old != nullptr) {
		QTimer::singleShot(0, [this]() { this->postReload(); });
	} else {
		this->postReload();
	}
}

void EngineGeneration::postReload() {
	QuickshellPlugin::runOnReload();
	PostReloadHook::postReloadTree(this->root);
	this->singletonRegistry.onPostReload();
}

void EngineGeneration::setWatchingFiles(bool watching) {
	if (watching) {
		if (this->watcher == nullptr) {
			this->watcher = new QFileSystemWatcher();

			for (auto& file: this->scanner.scannedFiles) {
				this->watcher->addPath(file);
			}

			QObject::connect(
			    this->watcher,
			    &QFileSystemWatcher::fileChanged,
			    this,
			    &EngineGeneration::filesChanged
			);
		}
	} else {
		if (this->watcher != nullptr) {
			delete this->watcher;
			this->watcher = nullptr;
		}
	}
}

void EngineGeneration::registerIncubationController(QQmlIncubationController* controller) {
	auto* obj = dynamic_cast<QObject*>(controller);

	// We only want controllers that we can swap out if destroyed.
	// This happens if the window owning the active controller dies.
	if (obj == nullptr) {
		qCDebug(logIncubator) << "Could not register incubation controller as it is not a QObject"
		                      << controller;

		return;
	}

	this->incubationControllers.push_back(controller);

	QObject::connect(
	    obj,
	    &QObject::destroyed,
	    this,
	    &EngineGeneration::incubationControllerDestroyed
	);

	qCDebug(logIncubator) << "Registered incubation controller" << controller;

	if (this->engine.incubationController() == &this->delayedIncubationController) {
		this->assignIncubationController();
	}
}

void EngineGeneration::incubationControllerDestroyed() {
	qCDebug(logIncubator) << "Active incubation controller destroyed, deregistering";

	this->incubationControllers.removeAll(dynamic_cast<QQmlIncubationController*>(this->sender()));
	this->assignIncubationController();
}

void EngineGeneration::assignIncubationController() {
	auto* controller = this->incubationControllers.first();
	if (controller == nullptr) controller = &this->delayedIncubationController;

	qCDebug(logIncubator) << "Assigning incubation controller to engine:" << controller
	                      << "fallback:" << (controller == &this->delayedIncubationController);

	this->engine.setIncubationController(controller);
}

EngineGeneration* EngineGeneration::findObjectGeneration(QObject* object) {
	while (object != nullptr) {
		auto* context = QQmlEngine::contextForObject(object);
		if (auto* generation = g_generations.value(context->engine())) {
			return generation;
		}
	}

	return nullptr;
}
