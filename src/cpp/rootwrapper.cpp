#include "rootwrapper.hpp"
#include <cstdlib>
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qurl.h>

#include "shell.hpp"

RootWrapper::RootWrapper(QUrl rootUrl):
    QObject(nullptr), rootUrl(std::move(rootUrl)), engine(this) {
	this->reloadGraph();

	if (this->activeRoot == nullptr) {
		qCritical() << "could not create scene graph, exiting";
		exit(-1); // NOLINT
	}
}

void RootWrapper::reloadGraph() {
	if (this->activeRoot != nullptr) {
		this->engine.clearComponentCache();
	}

	auto component = QQmlComponent(&this->engine, this->rootUrl);
	auto* obj = component.beginCreate(this->engine.rootContext());
	if (obj == nullptr) {
		qWarning() << "failed to create root component";
		return;
	}

	auto* qtsobj = qobject_cast<QtShell*>(obj);
	if (qtsobj == nullptr) {
		qWarning() << "root component was not a QtShell";
		delete obj;
		return;
	}

	component.completeCreate();

	if (this->activeRoot != nullptr) {
		this->activeRoot->deleteLater();
		this->activeRoot = nullptr;
	}

	this->activeRoot = qtsobj;
}

void RootWrapper::changeRoot(QtShell* newRoot) {
	if (this->activeRoot != nullptr) {
		QObject::disconnect(this->destroyConnection);
		this->activeRoot->deleteLater();
	}

	if (newRoot != nullptr) {
		this->activeRoot = newRoot;
		QObject::connect(this->activeRoot, &QtShell::destroyed, this, &RootWrapper::destroy);
	}
}

void RootWrapper::destroy() { this->deleteLater(); }
