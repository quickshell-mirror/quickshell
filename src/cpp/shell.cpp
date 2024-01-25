#include "shell.hpp"
#include <utility>

#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlcomponent.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

QString CONFIG_PATH; // NOLINT

QtShell::QtShell(): QObject(nullptr), path(CONFIG_PATH), dir(QFileInfo(this->path).dir()) {
	CONFIG_PATH = "";
}

void QtShell::componentComplete() {
	if (this->path.isEmpty()) {
		qWarning() << "Multiple QtShell objects were created. You should not do this.";
		return;
	}

	for (auto* component: this->mComponents) {
		component->prepare(this->dir);
	}
}

QQmlListProperty<ShellComponent> QtShell::components() {
	return QQmlListProperty<ShellComponent>(
	    this,
	    nullptr,
	    &QtShell::appendComponent,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr
	);
}

void QtShell::appendComponent(QQmlListProperty<ShellComponent>* list, ShellComponent* component) {
	auto* shell = qobject_cast<QtShell*>(list->object);

	if (shell != nullptr) shell->mComponents.append(component);
}

void ShellComponent::setSource(QString source) {
	if (this->mComponent != nullptr) {
		qWarning() << "cannot define ShellComponent.source while defining a component";
		return;
	}

	this->mSource = std::move(source);
}

void ShellComponent::setComponent(QQmlComponent* component) {
	if (this->mSource != nullptr) {
		qWarning() << "cannot define a component for ShellComponent while source is set";
		return;
	}

	this->mComponent = component;
}

void ShellComponent::prepare(const QDir& basePath) {
	if (this->mComponent == nullptr) {
		if (this->mSource == nullptr) {
			qWarning() << "neither source or a component was set for ShellComponent on prepare";
			return;
		}

		auto path = basePath.filePath(this->mSource);
		qDebug() << "preparing ShellComponent from" << path;

		auto* context = QQmlEngine::contextForObject(this);
		if (context == nullptr) {
			qWarning() << "ShellComponent was created without an associated QQmlEngine, cannot prepare";
			return;
		}

		auto* engine = context->engine();

		this->mComponent = new QQmlComponent(engine, path, this);

		if (this->mComponent == nullptr) {
			qWarning() << "could not load ShellComponent source" << path;
			return;
		}
	}

	qDebug() << "Sending ready for ShellComponent";
	emit this->ready();
}
