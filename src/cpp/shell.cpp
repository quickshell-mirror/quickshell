#include "shell.hpp"
#include <utility>

#include <qobject.h>
#include <qqmllist.h>

void QtShell::earlyInit(QObject* old) {
	auto* oldshell = qobject_cast<QtShell*>(old);

	if (oldshell != nullptr) {
		this->scavengeableChildren = std::move(oldshell->children);
	}
}

QObject* QtShell::scavengeTargetFor(QObject* /* child */) {
	if (this->scavengeableChildren.length() > this->children.length()) {
		return this->scavengeableChildren[this->children.length()];
	}

	return nullptr;
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
	shell->children.append(component);
}
