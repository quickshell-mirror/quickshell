#include "shell.hpp"
#include <utility>

#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

void ShellRoot::earlyInit(QObject* old) {
	auto* oldshell = qobject_cast<ShellRoot*>(old);

	if (oldshell != nullptr) {
		this->scavengeableChildren = std::move(oldshell->children);
	}
}

QObject* ShellRoot::scavengeTargetFor(QObject* /* child */) {
	if (this->scavengeableChildren.length() > this->children.length()) {
		return this->scavengeableChildren[this->children.length()];
	}

	return nullptr;
}

void ShellRoot::setConfig(ShellConfig config) {
	this->mConfig = config;

	emit this->configChanged();
}

ShellConfig ShellRoot::config() const { return this->mConfig; }

QQmlListProperty<QObject> ShellRoot::components() {
	return QQmlListProperty<QObject>(
	    this,
	    nullptr,
	    &ShellRoot::appendComponent,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr
	);
}

void ShellRoot::appendComponent(QQmlListProperty<QObject>* list, QObject* component) {
	auto* shell = static_cast<ShellRoot*>(list->object); // NOLINT
	component->setParent(shell);
	shell->children.append(component);
}
