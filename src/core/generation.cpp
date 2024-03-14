#include "generation.hpp"

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qtimer.h>

#include "plugin.hpp"
#include "reload.hpp"

static QHash<QQmlEngine*, EngineGeneration*> g_generations; // NOLINT

EngineGeneration::EngineGeneration() { g_generations.insert(&this->engine, this); }

EngineGeneration::~EngineGeneration() {
	g_generations.remove(&this->engine);
	if (this->root != nullptr) this->root->deleteLater();
}

void EngineGeneration::onReload(EngineGeneration* old) {
	auto* app = QCoreApplication::instance();
	QObject::connect(&this->engine, &QQmlEngine::quit, app, &QCoreApplication::quit);
	QObject::connect(&this->engine, &QQmlEngine::exit, app, &QCoreApplication::exit);

	this->root->onReload(old == nullptr ? nullptr : old->root);
	this->singletonRegistry.onReload(old == nullptr ? nullptr : &old->singletonRegistry);

	delete old;

	if (old != nullptr) {
		QTimer::singleShot(0, [this]() {
			QuickshellPlugin::runOnReload();
			PostReloadHook::postReloadTree(this->root);
		});
	} else {
		QuickshellPlugin::runOnReload();
		PostReloadHook::postReloadTree(this->root);
	}
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
