#include "singleton.hpp"

#include <qhash.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>

#include "generation.hpp"
#include "reload.hpp"

void Singleton::componentComplete() {
	auto* context = QQmlEngine::contextForObject(this);

	if (context == nullptr) {
		qWarning() << "Not registering singleton not created in the qml context:" << this;
		return;
	}

	auto url = context->baseUrl();

	if (this->parent() != nullptr || context->contextObject() != this) {
		qWarning() << "Tried to register singleton" << this
		           << "which is not the root component of its file" << url;
		return;
	}

	auto* generation = EngineGeneration::findObjectGeneration(this);

	if (generation == nullptr) {
		qWarning() << "Tried to register singleton" << this
		           << "which has no associated engine generation" << url;
		return;
	}

	generation->singletonRegistry.registerSingleton(url, this);
	this->ReloadPropagator::componentComplete();
}

void SingletonRegistry::registerSingleton(const QUrl& url, Singleton* singleton) {
	if (this->registry.contains(url)) {
		qWarning() << "Tried to register singleton twice for the same file" << url;
		return;
	}

	this->registry.insert(url, singleton);
}

void SingletonRegistry::onReload(SingletonRegistry* old) {
	for (auto [url, singleton]: this->registry.asKeyValueRange()) {
		singleton->reload(old == nullptr ? nullptr : old->registry.value(url));
	}
}
