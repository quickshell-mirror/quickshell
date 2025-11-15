#include "model.hpp"

#include <qhash.h>
#include <qbytearray.h>
#include <qnamespace.h>

QHash<int, QByteArray> UntypedObjectModel::roleNames() const {
	return {{Qt::UserRole, "modelData"}};
}

UntypedObjectModel* UntypedObjectModel::emptyInstance() {
	return ObjectModel<void>::emptyInstance();
}
