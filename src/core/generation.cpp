#include "generation.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlerror.h>
#include <qqmlincubator.h>
#include <qtmetamacros.h>

#include "iconimageprovider.hpp"
#include "imageprovider.hpp"
#include "incubator.hpp"
#include "logcat.hpp"
#include "plugin.hpp"
#include "qsintercept.hpp"
#include "reload.hpp"
#include "scan.hpp"

namespace {
QS_LOGGING_CATEGORY(logScene, "scene");
}

static QHash<const QQmlEngine*, EngineGeneration*> g_generations; // NOLINT

EngineGeneration::EngineGeneration(const QDir& rootPath, QmlScanner scanner)
    : rootPath(rootPath)
    , scanner(std::move(scanner))
    , urlInterceptor(this->rootPath)
    , interceptNetFactory(this->rootPath, this->scanner.fileIntercepts)
    , engine(new QQmlEngine()) {
	g_generations.insert(this->engine, this);

	this->engine->setOutputWarningsToStandardError(false);
	QObject::connect(this->engine, &QQmlEngine::warnings, this, &EngineGeneration::onEngineWarnings);

	this->engine->addUrlInterceptor(&this->urlInterceptor);
	this->engine->addImportPath("qs:@/");

	this->engine->setNetworkAccessManagerFactory(&this->interceptNetFactory);
	this->engine->setIncubationController(&this->delayedIncubationController);

	this->engine->addImageProvider("icon", new IconImageProvider());
	this->engine->addImageProvider("qsimage", new QsImageProvider());
	this->engine->addImageProvider("qspixmap", new QsPixmapProvider());

	QsEnginePlugin::runConstructGeneration(*this);
}

EngineGeneration::EngineGeneration(): EngineGeneration(QDir(), QmlScanner()) {}

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
		qCDebug(logIncubator) << "Locking incubation controllers of old generation" << old;
		old->incubationControllersLocked = true;
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

	emit this->firePostReload();
	QObject::disconnect(this, &EngineGeneration::firePostReload, nullptr, nullptr);
}

void EngineGeneration::setWatchingFiles(bool watching) {
	if (watching) {
		if (this->watcher == nullptr) {
			this->watcher = new QFileSystemWatcher();

			for (auto& file: this->scanner.scannedFiles) {
				this->watcher->addPath(file);
				this->watcher->addPath(QFileInfo(file).dir().absolutePath());
			}

			for (auto& file: this->extraWatchedFiles) {
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
			this->watcher->deleteLater();
			this->watcher = nullptr;
		}
	}
}

bool EngineGeneration::setExtraWatchedFiles(const QVector<QString>& files) {
	this->extraWatchedFiles.clear();
	for (const auto& file: files) {
		if (!this->scanner.scannedFiles.contains(file)) {
			this->extraWatchedFiles.append(file);
		}
	}

	if (this->watcher) {
		this->setWatchingFiles(false);
		this->setWatchingFiles(true);
	}

	return !this->extraWatchedFiles.isEmpty();
}

void EngineGeneration::onFileChanged(const QString& name) {
	if (!this->watcher->files().contains(name)) {
		this->deletedWatchedFiles.push_back(name);
	} else {
		// some editors (e.g vscode) perform file saving in two steps: truncate + write
		// ignore the first event (truncate) with size 0 to prevent incorrect live reloading
		auto fileInfo = QFileInfo(name);
		if (fileInfo.isFile() && fileInfo.size() == 0) return;

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
	// We only want controllers that we can swap out if destroyed.
	// This happens if the window owning the active controller dies.
	auto* obj = dynamic_cast<QObject*>(controller);
	if (!obj) {
		qCWarning(logIncubator) << "Could not register incubation controller as it is not a QObject"
		                        << controller;

		return;
	}

	QObject::connect(
	    obj,
	    &QObject::destroyed,
	    this,
	    &EngineGeneration::incubationControllerDestroyed,
	    Qt::UniqueConnection
	);

	this->incubationControllers.push_back(obj);
	qCDebug(logIncubator) << "Registered incubation controller" << obj << "to generation" << this;

	// This function can run during destruction.
	if (this->engine == nullptr) return;

	if (this->engine->incubationController() == &this->delayedIncubationController) {
		this->assignIncubationController();
	}
}

// Multiple controllers may be destroyed at once. Dynamic casts must be performed before working
// with any controllers. The QQmlIncubationController destructor will already have run by the
// point QObject::destroyed is called, so we can't cast to that.
void EngineGeneration::deregisterIncubationController(QQmlIncubationController* controller) {
	auto* obj = dynamic_cast<QObject*>(controller);
	if (!obj) {
		qCCritical(logIncubator) << "Deregistering incubation controller which is not a QObject, "
		                            "however only QObject controllers should be registered.";
	}

	QObject::disconnect(obj, nullptr, this, nullptr);

	if (this->incubationControllers.removeOne(obj)) {
		qCDebug(logIncubator) << "Deregistered incubation controller" << obj << "from" << this;
	} else {
		qCCritical(logIncubator) << "Failed to deregister incubation controller" << obj << "from"
		                         << this << "as it was not registered to begin with";
		qCCritical(logIncubator) << "Current registered incuabation controllers"
		                         << this->incubationControllers;
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

	if (this->incubationControllers.removeAll(sender) != 0) {
		qCDebug(logIncubator) << "Destroyed incubation controller" << sender << "deregistered from"
		                      << this;
	} else {
		qCCritical(logIncubator) << "Destroyed incubation controller" << sender
		                         << "was not registered, but its destruction was observed by" << this;

		return;
	}

	// This function can run during destruction.
	if (this->engine == nullptr) return;

	if (dynamic_cast<QObject*>(this->engine->incubationController()) == sender) {
		qCDebug(logIncubator
		) << "Destroyed incubation controller was currently active, reassigning from pool";
		this->assignIncubationController();
	}
}

void EngineGeneration::onEngineWarnings(const QList<QQmlError>& warnings) {
	for (const auto& error: warnings) {
		const auto& url = error.url();
		auto rel = url.scheme() == "qs" && url.path().startsWith("@/qs/") ? "@" % url.path().sliced(5)
		                                                                  : url.toString();

		QString objectName;
		auto desc = error.description();
		if (auto i = desc.indexOf(": "); i != -1 && desc.startsWith("QML ")) {
			objectName = desc.first(i) + " at ";
			desc = desc.sliced(i + 2);
		}

		qCWarning(logScene).noquote().nospace()
		    << objectName << rel << '[' << error.line() << ':' << error.column() << "]: " << desc;
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

	if (this->incubationControllersLocked || this->incubationControllers.isEmpty()) {
		controller = &this->delayedIncubationController;
	} else {
		controller = dynamic_cast<QQmlIncubationController*>(this->incubationControllers.first());
	}

	qCDebug(logIncubator) << "Assigning incubation controller" << controller << "to generation"
	                      << this
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
	// Objects can still attempt to find their generation after it has been destroyed.
	// if (g_generations.size() == 1) return EngineGeneration::currentGeneration();

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
