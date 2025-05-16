#include "wrapper.hpp"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmllist.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qquickitem.h>
#include <qtmetamacros.h>

namespace qs::widgets {

void WrapperManager::componentComplete() {
	if (this->mAssignedWrapper) {
		this->mWrapper = this->mAssignedWrapper;
	} else {
		this->mWrapper = qobject_cast<QQuickItem*>(this->parent());
	}

	if (!this->mWrapper) {
		QString pstr;
		QDebug(&pstr) << this->parent();

		qmlWarning(this) << "Parent of WrapperManager is not a QQuickItem. Parent: " << pstr;
		return;
	}

	QQuickItem* child = this->mChild;
	this->mChild = nullptr; // avoids checks for the old item in setChild.

	const auto& childItems = this->mWrapper->childItems();

	if (childItems.length() == 1) {
		this->mDefaultChild = childItems.first();
		if (!child) child = this->mDefaultChild;
	} else if (!childItems.empty()) {
		this->flags.setFlag(WrapperManager::HasMultipleChildren);

		if (!child && !this->flags.testFlags(WrapperManager::NullChild)) {
			this->printChildCountWarning();
		}
	}

	for (auto* item: childItems) {
		if (item != child) item->setParentItem(nullptr);
	}

	if (child && !this->flags.testFlag(WrapperManager::NullChild)) {
		this->setChild(child);
	}
}

QQuickItem* WrapperManager::child() const { return this->mChild; }

void WrapperManager::setChild(QQuickItem* child) {
	if (child && child == this->mChild) return;

	if (this->mChild != nullptr) {
		QObject::disconnect(this->mChild, nullptr, this, nullptr);

		if (this->mChild->parentItem() == this->mWrapper) {
			this->mChild->setParentItem(nullptr);
		}
		this->disconnectChild();
	}

	this->mChild = child;
	this->flags.setFlag(WrapperManager::NullChild, child == nullptr);

	if (child) {
		QObject::connect(
		    child,
		    &QObject::destroyed,
		    this,
		    &WrapperManager::onChildDestroyed,
		    Qt::UniqueConnection
		);

		if (auto* wrapper = this->mWrapper) {
			child->setParentItem(wrapper);
		}

		this->connectChild();
	}

	emit this->initializedChildChanged();
	emit this->childChanged();
}

void WrapperManager::setProspectiveChild(QQuickItem* child) {
	if (child && child == this->mChild) return;

	if (!this->mWrapper) {
		if (this->mChild) {
			QObject::disconnect(this->mChild, nullptr, this, nullptr);
		}

		this->mChild = child;
		this->flags.setFlag(WrapperManager::NullChild, child == nullptr);

		if (child) {
			QObject::connect(child, &QObject::destroyed, this, &WrapperManager::onChildDestroyed);
		}
	} else {
		this->setChild(child);
	}
}

void WrapperManager::unsetChild() {
	if (!this->mWrapper) {
		this->setProspectiveChild(nullptr);
	} else {
		this->setChild(this->mDefaultChild);

		if (!this->mDefaultChild && this->flags.testFlag(WrapperManager::HasMultipleChildren)) {
			this->printChildCountWarning();
		}
	}

	this->flags.setFlag(WrapperManager::NullChild, false);
}

void WrapperManager::onChildDestroyed() {
	this->mChild = nullptr;
	this->unsetChild();
	emit this->childChanged();
}

QQuickItem* WrapperManager::wrapper() const { return this->mWrapper; }

void WrapperManager::setWrapper(QQuickItem* wrapper) {
	if (this->mWrapper) {
		qmlWarning(this) << "Cannot set wrapper after WrapperManager initialization.";
		return;
	}

	this->mAssignedWrapper = wrapper;
}

void WrapperManager::printChildCountWarning() const {
	qmlWarning(this->mWrapper) << "Wrapper component cannot have more than one visual child.";
	qmlWarning(this->mWrapper) << "Remove all additional children, or pick a specific component "
	                              "to wrap using the child property.";
}

} // namespace qs::widgets
