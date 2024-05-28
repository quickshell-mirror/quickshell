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
#include <qtimer.h>
#include <qtmetamacros.h>

#include "iconimageprovider.hpp"
#include "imageprovider.hpp"
#include "incubator.hpp"
#include "plugin.hpp"
#include "qsintercept.hpp"
#include "reload.hpp"
#include "scan.hpp"

static QHash<QQmlEngine*, EngineGeneration*> g_generations; // NOLINT

EngineGeneration::EngineGeneration(QmlScanner scanner)
    : scanner(std::move(scanner))
    , interceptNetFactory(this->scanner.qmldirIntercepts)
    , engine(new QQmlEngine()) {
	g_generations.insert(this->engine, this);

	this->engine->addUrlInterceptor(&this->urlInterceptor);
	this->engine->setNetworkAccessManagerFactory(&this->interceptNetFactory);
	this->engine->setIncubationController(&this->delayedIncubationController);

	this->engine->addImageProvider("icon", new IconImageProvider());
	this->engine->addImageProvider("qsimage", new QsImageProvider());
	this->engine->addImageProvider("qspixmap", new QsPixmapProvider());

	QuickshellPlugin::runConstructGeneration(*this);
}

EngineGeneration::~EngineGeneration() {
	g_generations.remove(this->engine);
	delete this->engine;
}

void EngineGeneration::destroy() {
	// Multiple generations can detect a reload at the same time.
	delete this->watcher;
	this->watcher = nullptr;

	// Yes all of this is actually necessary.
	if (this->engine != nullptr && this->root != nullptr) {
		QObject::connect(this->root, &QObject::destroyed, this, [this]() {
			// The timer seems to fix *one* of the possible qml item destructor crashes.
			QTimer::singleShot(0, [this]() {
				// Garbage is not collected during engine destruction.
				this->engine->collectGarbage();

				QObject::connect(this->engine, &QObject::destroyed, this, [this]() { delete this; });

				// Even after all of that there's still multiple failing assertions and segfaults.
				// Pray you don't hit one.
				// Note: it appeats *some* of the crashes are related to values owned by the generation.
				// Test by commenting the connect() above.
				this->engine->deleteLater();
				this->engine = nullptr;
			});
		});

		this->root->deleteLater();
		this->root = nullptr;
	}
}

void EngineGeneration::onReload(EngineGeneration* old) {
	if (old != nullptr) {
		// if the old generation holds the window incubation controller as the
		// new generation acquires it then incubators will hang intermittently
		old->incubationControllers.clear();
		old->assignIncubationController();
	}

	auto* app = QCoreApplication::instance();
	QObject::connect(this->engine, &QQmlEngine::quit, app, &QCoreApplication::quit);
	QObject::connect(this->engine, &QQmlEngine::exit, app, &QCoreApplication::exit);

	this->root->reload(old == nullptr ? nullptr : old->root);
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

void EngineGeneration::assignIncubationController() {
	QQmlIncubationController* controller = nullptr;
	if (this->incubationControllers.isEmpty()) controller = &this->delayedIncubationController;
	else controller = this->incubationControllers.first().first;

	qCDebug(logIncubator) << "Assigning incubation controller to engine:" << controller
	                      << "fallback:" << (controller == &this->delayedIncubationController);

	this->engine->setIncubationController(controller);
}

EngineGeneration* EngineGeneration::findObjectGeneration(QObject* object) {
	while (object != nullptr) {
		auto* context = QQmlEngine::contextForObject(object);

		if (context != nullptr) {
			if (auto* generation = g_generations.value(context->engine())) {
				return generation;
			}
		}

		object = object->parent();
	}

	return nullptr;
}
