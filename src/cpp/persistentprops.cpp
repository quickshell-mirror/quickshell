#include "persistentprops.hpp"

#include <qobject.h>
#include <qtmetamacros.h>

void PersistentProperties::onReload(QObject* oldInstance) {
	if (qobject_cast<PersistentProperties*>(oldInstance) == nullptr) {
		emit this->loaded();
		return;
	}

	const auto* metaObject = this->metaObject();
	for (auto i = metaObject->propertyOffset(); i < metaObject->propertyCount(); i++) {
		const auto prop = metaObject->property(i);
		auto oldProp = oldInstance->property(prop.name());

		if (oldProp.isValid()) {
			this->setProperty(prop.name(), oldProp);
		}
	}

	emit this->loaded();
	emit this->reloaded();
}
