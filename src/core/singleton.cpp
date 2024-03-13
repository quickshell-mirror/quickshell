#include "singleton.hpp"

#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qurl.h>

#include "reload.hpp"

void Singleton::componentComplete() {
	auto* context = QQmlEngine::contextForObject(this);

	if (context == nullptr) {
		qWarning() << "not registering singleton not created in the qml context:" << this;
		return;
	}

	auto url = context->baseUrl();

	if (this->parent() != nullptr || context->contextObject() != this) {
		qWarning() << "tried to register singleton" << this
		           << "which is not the root component of its file" << url;
		return;
	}

	SingletonRegistry::instance()->install(url, this);
	this->ReloadPropagator::componentComplete();
}

SingletonRegistry::~SingletonRegistry() {
	delete this->previousRegistry;
	delete this->currentRegistry;
}

void SingletonRegistry::install(const QUrl& url, Singleton* singleton) {
	QObject* old = nullptr;

	if (this->previousRegistry != nullptr) {
		old = this->previousRegistry->value(url);
	}

	if (this->currentRegistry == nullptr) {
		this->currentRegistry = new QMap<QUrl, QObject*>();
	}

	this->currentRegistry->insert(url, singleton);
	singleton->onReload(old);
}

void SingletonRegistry::flip() {
	delete this->previousRegistry;
	this->previousRegistry = this->currentRegistry;
	this->currentRegistry = nullptr;
}

SingletonRegistry* SingletonRegistry::instance() {
	static SingletonRegistry* instance = nullptr; // NOLINT
	if (instance == nullptr) {
		instance = new SingletonRegistry();
	}
	return instance;
}
