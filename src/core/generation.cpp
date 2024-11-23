#include "generation.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlincubator.h>
#include <qtmetamacros.h>

#include "iconimageprovider.hpp"
#include "imageprovider.hpp"
#include "incubator.hpp"
#include "plugin.hpp"
#include "qsintercept.hpp"
#include "reload.hpp"
#include "scan.hpp"

static QHash<const QQmlEngine*, EngineGeneration*> g_generations; // NOLINT

EngineGeneration::EngineGeneration(const QDir& rootPath, QmlScanner scanner)
    : rootPath(rootPath)
    , scanner(std::move(scanner))
    , urlInterceptor(this->rootPath)
    , interceptNetFactory(this->scanner.qmldirIntercepts)
    , engine(new QQmlEngine()) {
	g_generations.insert(this->engine, this);

	this->engine->addUrlInterceptor(&this->urlInterceptor);
	this->engine->setNetworkAccessManagerFactory(&this->interceptNetFactory);
	this->engine->setIncubationController(&this->delayedIncubationController);

	this->engine->addImageProvider("icon", new IconImageProvider());
	this->engine->addImageProvider("qsimage", new QsImageProvider());
	this->engine->addImageProvider("qspixmap", new QsPixmapProvider());

	QsEnginePlugin::runConstructGeneration(*this);
}

EngineGeneration::~EngineGeneration() {
	if (this->engine != nullptr) {
		qFatal() << this << "destroyed without calling destroy()";
	}
}

void EngineGeneration::destroy() {
	if (this->destroying) return;
	this->destroying = true;

	if (this->watcher != nullptr) {
		// Multiple generations can detect a reload at the same time.
		QObject::disconnect(this->watcher, nullptr, this, nullptr);
		this->watcher->deleteLater();
		this->watcher = nullptr;
	}

	for (auto* extension: this->extensions.values()) {
		delete extension;
	}

	if (this->root != nullptr) {
		QObject::connect(this->root, &QObject::destroyed, this, [this]() {
			// prevent further js execution between garbage collection and engine destruction.
			this->engine->setInterrupted(true);

			g_generations.remove(this->engine);

			// Garbage is not collected during engine destruction.
			this->engine->collectGarbage();

			delete this->engine;
			this->engine = nullptr;

			auto terminate = this->shouldTerminate;
			auto code = this->exitCode;
			delete this;

			if (terminate) QCoreApplication::exit(code);
		});

		this->root->deleteLater();
		this->root = nullptr;
	} else {
		g_generations.remove(this->engine);

		// the engine has never been used, no need to clean up
		delete this->engine;
		this->engine = nullptr;

		auto terminate = this->shouldTerminate;
		auto code = this->exitCode;
		delete this;

		if (terminate) QCoreApplication::exit(code);
	}
}

void EngineGeneration::shutdown() {
	if (this->destroying) return;

	delete this->root;
	this->root = nullptr;
	delete this->engine;
	this->engine = nullptr;
	delete this;
}

void EngineGeneration::onReload(EngineGeneration* old) {
	if (old != nullptr) {
		// if the old generation holds the window incubation controller as the
		// new generation acquires it then incubators will hang intermittently
		old->incubationControllers.clear();
		old->assignIncubationController();
	}

	QObject::connect(this->engine, &QQmlEngine::quit, this, &EngineGeneration::quit);
	QObject::connect(this->engine, &QQmlEngine::exit, this, &EngineGeneration::exit);

	if (auto* reloadable = qobject_cast<Reloadable*>(this->root)) {
		reloadable->reload(old ? old->root : nullptr);
	}

	this->singletonRegistry.onReload(old == nullptr ? nullptr : &old->singletonRegistry);
	this->reloadComplete = true;
	emit this->reloadFinished();

	if (old != nullptr) {
		QObject::connect(old, &QObject::destroyed, this, [this]() { this->postReload(); });
		old->destroy();
	} else {
		this->postReload();
	}
}

void EngineGeneration::postReload() {
	// This can be called on a generation during its destruction.
	if (this->engine == nullptr || this->root == nullptr) return;

	QsEnginePlugin::runOnReload();
	PostReloadHook::postReloadTree(this->root);
	this->singletonRegistry.onPostReload();
}

void EngineGeneration::setWatchingFiles(bool watching) {
	if (watching) {
		if (this->watcher == nullptr) {
			this->watcher = new QFileSystemWatcher();

			for (auto& file: this->scanner.scannedFiles) {
				this->watcher->addPath(file);
				this->watcher->addPath(QFileInfo(file).dir().absolutePath());
			}

			QObject::connect(
			    this->watcher,
			    &QFileSystemWatcher::fileChanged,
			    this,
			    &EngineGeneration::onFileChanged
			);

			QObject::connect(
			    this->watcher,
			    &QFileSystemWatcher::directoryChanged,
			    this,
			    &EngineGeneration::onDirectoryChanged
			);
		}
	} else {
		if (this->watcher != nullptr) {
			delete this->watcher;
			this->watcher = nullptr;
		}
	}
}

void EngineGeneration::onFileChanged(const QString& name) {
	if (!this->watcher->files().contains(name)) {
		this->deletedWatchedFiles.push_back(name);
	} else {
		emit this->filesChanged();
	}
}

void EngineGeneration::onDirectoryChanged() {
	// try to find any files that were just deleted from a replace operation
	for (auto& file: this->deletedWatchedFiles) {
		if (QFileInfo(file).exists()) {
			emit this->filesChanged();
			break;
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

	this->incubationControllers.push_back({controller, obj});

	QObject::connect(
	    obj,
	    &QObject::destroyed,
	    this,
	    &EngineGeneration::incubationControllerDestroyed
	);

	qCDebug(logIncubator) << "Registered incubation controller" << controller;

	// This function can run during destruction.
	if (this->engine == nullptr) return;

	if (this->engine->incubationController() == &this->delayedIncubationController) {
		this->assignIncubationController();
	}
}

void EngineGeneration::deregisterIncubationController(QQmlIncubationController* controller) {
	QObject* obj = nullptr;
	this->incubationControllers.removeIf([&](QPair<QQmlIncubationController*, QObject*> other) {
		if (controller == other.first) {
			obj = other.second;
			return true;
		} else return false;
	});

	if (obj == nullptr) {
		qCWarning(logIncubator) << "Failed to deregister incubation controller" << controller
		                        << "as it was not registered to begin with";
		qCWarning(logIncubator) << "Current registered incuabation controllers"
		                        << this->incubationControllers;
	} else {
		QObject::disconnect(obj, nullptr, this, nullptr);
		qCDebug(logIncubator) << "Deregistered incubation controller" << controller;
	}

	// This function can run during destruction.
	if (this->engine == nullptr) return;

	if (this->engine->incubationController() == controller) {
		qCDebug(logIncubator
		) << "Destroyed incubation controller was currently active, reassigning from pool";
		this->assignIncubationController();
	}
}

void EngineGeneration::incubationControllerDestroyed() {
	auto* sender = this->sender();
	QQmlIncubationController* controller = nullptr;

	this->incubationControllers.removeIf([&](QPair<QQmlIncubationController*, QObject*> other) {
		if (sender == other.second) {
			controller = other.first;
			return true;
		} else return false;
	});

	if (controller == nullptr) {
		qCCritical(logIncubator) << "Destroyed incubation controller" << this->sender()
		                         << "could not be identified, this may cause memory corruption";
		qCCritical(logIncubator) << "Current registered incuabation controllers"
		                         << this->incubationControllers;
	} else {
		qCDebug(logIncubator) << "Destroyed incubation controller" << controller << "deregistered";
	}

	// This function can run during destruction.
	if (this->engine == nullptr) return;

	if (this->engine->incubationController() == controller) {
		qCDebug(logIncubator
		) << "Destroyed incubation controller was currently active, reassigning from pool";
		this->assignIncubationController();
	}
}

void EngineGeneration::registerExtension(const void* key, EngineGenerationExt* extension) {
	if (this->extensions.contains(key)) {
		delete this->extensions.value(key);
	}

	this->extensions.insert(key, extension);
}

EngineGenerationExt* EngineGeneration::findExtension(const void* key) {
	return this->extensions.value(key);
}

void EngineGeneration::quit() {
	this->shouldTerminate = true;
	this->destroy();
}

void EngineGeneration::exit(int code) {
	this->shouldTerminate = true;
	this->exitCode = code;
	this->destroy();
}

void EngineGeneration::assignIncubationController() {
	QQmlIncubationController* controller = nullptr;
	if (this->incubationControllers.isEmpty()) controller = &this->delayedIncubationController;
	else controller = this->incubationControllers.first().first;

	qCDebug(logIncubator) << "Assigning incubation controller to engine:" << controller
	                      << "fallback:" << (controller == &this->delayedIncubationController);

	this->engine->setIncubationController(controller);
}

EngineGeneration* EngineGeneration::currentGeneration() {
	if (g_generations.size() == 1) {
		return *g_generations.begin();
	} else return nullptr;
}

EngineGeneration* EngineGeneration::findEngineGeneration(const QQmlEngine* engine) {
	return g_generations.value(engine);
}

EngineGeneration* EngineGeneration::findObjectGeneration(const QObject* object) {
	if (g_generations.size() == 1) return EngineGeneration::currentGeneration();

	while (object != nullptr) {
		auto* context = QQmlEngine::contextForObject(object);

		if (context != nullptr) {
			if (auto* generation = EngineGeneration::findEngineGeneration(context->engine())) {
				return generation;
			}
		}

		object = object->parent();
	}

	return nullptr;
}
