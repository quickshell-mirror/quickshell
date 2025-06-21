#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::windowsystem {

class WorkspaceModel: public QObject {
	Q_OBJECT;
	QML_ELEMENT;

public:
	enum ConflictStrategy : quint8 {
		KeepFirst = 0,
		ShowDuplicates,
	};
	Q_ENUM(ConflictStrategy);

signals:
	void fromChanged();
	void toChanged();
	void screensChanged();
	void conflictStrategyChanged();

private:
	Q_OBJECT_BINDABLE_PROPERTY(WorkspaceModel, qint32, bFrom, &WorkspaceModel::fromChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WorkspaceModel, qint32, bTo, &WorkspaceModel::toChanged);
	Q_OBJECT_BINDABLE_PROPERTY(
	    WorkspaceModel,
	    ConflictStrategy,
	    bConflictStrategy,
	    &WorkspaceModel::conflictStrategyChanged
	);
};

} // namespace qs::windowsystem
