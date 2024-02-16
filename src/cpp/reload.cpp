#include "reload.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmllist.h>

void ReloadPropagator::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<ReloadPropagator*>(oldInstance);

	for (auto i = 0; i < this->mChildren.length(); i++) {
		auto* newChild = qobject_cast<Reloadable*>(this->mChildren.at(i));
		if (newChild != nullptr) {
			auto* oldChild = old == nullptr || old->mChildren.length() <= i
			                   ? nullptr
			                   : qobject_cast<Reloadable*>(old->mChildren.at(i));
			newChild->onReload(oldChild);
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
		reloadable->onReload(oldInstance);
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
