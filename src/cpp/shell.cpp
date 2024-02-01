#include "shell.hpp"

#include <qlogging.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>

#include "rootwrapper.hpp"

void QtShell::reload() {
	auto* rootobj = QQmlEngine::contextForObject(this)->engine()->parent();
	auto* root = qobject_cast<RootWrapper*>(rootobj);

	if (root == nullptr) {
		qWarning() << "cannot find RootWrapper for reload, ignoring request";
		return;
	}

	root->reloadGraph();
}

QQmlListProperty<QObject> QtShell::components() {
	return QQmlListProperty<QObject>(
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

void QtShell::appendComponent(QQmlListProperty<QObject>* list, QObject* component) {
	auto* shell = static_cast<QtShell*>(list->object); // NOLINT
	component->setParent(shell);
}
