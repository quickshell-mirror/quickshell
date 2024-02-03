#include "scavenge.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlengine.h>

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
