#include "reload.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtimer.h>

#include "generation.hpp"

void Reloadable::componentComplete() {
	this->engineGeneration = EngineGeneration::findObjectGeneration(this);

	if (this->engineGeneration != nullptr) {
		// When called this way there is no chance a reload will have old data,
		// but this will at least help prevent weird behaviors due to never getting a reload.
		if (this->engineGeneration->reloadComplete) {
			// Delayed due to Component.onCompleted running after QQmlParserStatus::componentComplete.
			QTimer::singleShot(0, this, &Reloadable::onReloadFinished);

			// This only matters for preventing the above timer from UAFing the generation,
			// so it isn't connected anywhere else.
			QObject::connect(
			    this->engineGeneration,
			    &QObject::destroyed,
			    this,
			    &Reloadable::onGenerationDestroyed
			);
		} else {
			QObject::connect(
			    this->engineGeneration,
			    &EngineGeneration::reloadFinished,
			    this,
			    &Reloadable::onReloadFinished
			);
		}
	}
}

void Reloadable::reload(QObject* oldInstance) {
	if (this->reloadComplete) return;
	this->onReload(oldInstance);
	this->reloadComplete = true;

	if (this->engineGeneration != nullptr) {
		QObject::disconnect(
		    this->engineGeneration,
		    &EngineGeneration::reloadFinished,
		    this,
		    &Reloadable::onReloadFinished
		);
	}
}

void Reloadable::onReloadFinished() { this->reload(nullptr); }
void Reloadable::onGenerationDestroyed() { this->engineGeneration = nullptr; }

void ReloadPropagator::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<ReloadPropagator*>(oldInstance);

	for (auto i = 0; i < this->mChildren.length(); i++) {
		auto* newChild = qobject_cast<Reloadable*>(this->mChildren.at(i));
		if (newChild != nullptr) {
			auto* oldChild = old == nullptr || old->mChildren.length() <= i
			                   ? nullptr
			                   : qobject_cast<Reloadable*>(old->mChildren.at(i));
			newChild->reload(oldChild);
		} else {
			Reloadable::reloadRecursive(newChild, oldInstance);
		}
	}
}

QQmlListProperty<QObject> ReloadPropagator::data() {
	return QQmlListProperty<QObject>(
	    this,
	    nullptr,
	    &ReloadPropagator::appendComponent,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr
	);
}

void ReloadPropagator::appendComponent(QQmlListProperty<QObject>* list, QObject* obj) {
	auto* self = static_cast<ReloadPropagator*>(list->object); // NOLINT
	obj->setParent(self);
	self->mChildren.append(obj);
}

void Reloadable::reloadRecursive(QObject* newObj, QObject* oldRoot) {
	auto* reloadable = qobject_cast<Reloadable*>(newObj);
	if (reloadable != nullptr) {
		QObject* oldInstance = nullptr;
		if (oldRoot != nullptr && !reloadable->mReloadableId.isEmpty()) {
			oldInstance = Reloadable::getChildByReloadId(oldRoot, reloadable->mReloadableId);
		}

		// pass handling to the child's onReload, which should call back into reloadRecursive,
		// with its oldInstance becoming the new oldRoot.
		reloadable->reload(oldInstance);
	} else if (newObj != nullptr) {
		Reloadable::reloadChildrenRecursive(newObj, oldRoot);
	}
}

void Reloadable::reloadChildrenRecursive(QObject* newRoot, QObject* oldRoot) {
	for (auto* child: newRoot->children()) {
		Reloadable::reloadRecursive(child, oldRoot);
	}
}

QObject* Reloadable::getChildByReloadId(QObject* parent, const QString& reloadId) {
	for (auto* child: parent->children()) {
		auto* reloadable = qobject_cast<Reloadable*>(child);
		if (reloadable != nullptr) {
			if (reloadable->mReloadableId == reloadId) return reloadable;
			// if not then don't check its children as thats a seperate reload scope.
		} else {
			auto* reloadable = Reloadable::getChildByReloadId(child, reloadId);
			if (reloadable != nullptr) return reloadable;
		}
	}

	return nullptr;
}

void PostReloadHook::componentComplete() {
	auto* engineGeneration = EngineGeneration::findObjectGeneration(this);
	if (!engineGeneration || engineGeneration->reloadComplete) this->postReload();
	else {
		// disconnected by EngineGeneration::postReload
		QObject::connect(
		    engineGeneration,
		    &EngineGeneration::firePostReload,
		    this,
		    &PostReloadHook::postReload
		);
	}
}

void PostReloadHook::postReload() {
	this->isPostReload = true;
	this->onPostReload();
}
