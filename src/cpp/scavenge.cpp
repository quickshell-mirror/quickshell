#include "scavenge.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>
#include <qqmllist.h>

// FIXME: there are core problems with SCAVENGE_PARENT due to the qml engine liking to set parents really late.
// this should instead be handled by proxying all property values until a possible target is ready or definitely not coming.
// The parent should probably be stable in componentComplete() but should be tested.

QObject* SCAVENGE_PARENT = nullptr; // NOLINT

void Scavenger::classBegin() {
	// prayers
	if (this->parent() == nullptr) {
		this->setParent(SCAVENGE_PARENT);
		SCAVENGE_PARENT = nullptr;
	}

	auto* parent = dynamic_cast<Scavengeable*>(this->parent());

	QObject* old = nullptr;
	if (parent != nullptr) {
		old = parent->scavengeTargetFor(this);
	}

	this->earlyInit(old);
}

QObject* createComponentScavengeable(
    QObject& parent,
    QQmlComponent& component,
    QVariantMap& initialProperties
) {
	SCAVENGE_PARENT = &parent;
	auto* instance = component.beginCreate(QQmlEngine::contextForObject(&parent));
	SCAVENGE_PARENT = nullptr;
	if (instance == nullptr) return nullptr;
	if (instance->parent() != nullptr) instance->setParent(&parent);
	component.setInitialProperties(instance, initialProperties);
	component.completeCreate();

	if (instance == nullptr) {
		qWarning() << component.errorString().toStdString().c_str();
	}

	return instance;
}

void ScavengeableScope::earlyInit(QObject* old) {
	auto* oldshell = qobject_cast<ScavengeableScope*>(old);

	if (oldshell != nullptr) {
		this->scavengeableData = std::move(oldshell->mData);
	}
}

QObject* ScavengeableScope::scavengeTargetFor(QObject* /* child */) {
	if (this->scavengeableData.length() > this->mData.length()) {
		return this->scavengeableData[this->mData.length()];
	}

	return nullptr;
}

QQmlListProperty<QObject> ScavengeableScope::data() {
	return QQmlListProperty<QObject>(
	    this,
	    nullptr,
	    &ScavengeableScope::appendComponent,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr
	);
}

void ScavengeableScope::appendComponent(QQmlListProperty<QObject>* list, QObject* component) {
	auto* self = static_cast<ScavengeableScope*>(list->object); // NOLINT
	component->setParent(self);
	self->mData.append(component);
}
