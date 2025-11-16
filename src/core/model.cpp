#include "model.hpp"

#include <qbytearray.h>
#include <qhash.h>
#include <qnamespace.h>

QHash<int, QByteArray> UntypedObjectModel::roleNames() const {
	return {{Qt::UserRole, "modelData"}};
}

UntypedObjectModel* UntypedObjectModel::emptyInstance() {
	static auto* instance = new ObjectModel<void>(nullptr);
	return instance;
}
