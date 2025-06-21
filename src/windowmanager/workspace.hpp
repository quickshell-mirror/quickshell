#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::wm {

Q_DECLARE_LOGGING_CATEGORY(logWorkspace);

class WorkspaceGroup: public QObject {
	Q_OBJECT;

private:
	Q_OBJECT_BINDABLE_PROPERTY(WorkspaceGroup, QList<QScreen>, bScreens);
};

class Workspace: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	// persistent id
	Q_PROPERTY(QString id READ default BINDABLE bindableId NOTIFY idChanged);
	Q_PROPERTY(QString name READ default BINDABLE bindableName NOTIFY nameChanged);
	// currently visible
	Q_PROPERTY(bool active READ default BINDABLE bindableActive NOTIFY activeChanged);
	// in workspace pickers
	Q_PROPERTY(bool shouldDisplay READ default BINDABLE bindableShouldDisplay NOTIFY shouldDisplayChanged);
	Q_PROPERTY(bool urgent READ default BINDABLE bindableUrgent NOTIFY urgentChanged);
	Q_PROPERTY(bool canActivate READ default BINDABLE bindableCanActivate NOTIFY canActivateChanged);
	Q_PROPERTY(bool canDeactivate READ default BINDABLE bindableCanDeactivate NOTIFY canDeactivateChanged);
	Q_PROPERTY(bool canRemove READ default BINDABLE bindableCanRemove NOTIFY canRemoveChanged);
	Q_PROPERTY(bool canSetGroup READ default BINDABLE bindableCanSetGroup NOTIFY canSetGroupChanged);
	// clang-format on

public:
	explicit Workspace(QObject* parent): QObject(parent) {}

	Q_INVOKABLE virtual void activate();
	Q_INVOKABLE virtual void deactivate();
	Q_INVOKABLE virtual void remove();

	[[nodiscard]] QBindable<QString> bindableId() const { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }
	[[nodiscard]] QBindable<bool> bindableActive() const { return &this->bActive; }
	[[nodiscard]] QBindable<bool> bindableShouldDisplay() const { return &this->bShouldDisplay; }
	[[nodiscard]] QBindable<bool> bindableUrgent() const { return &this->bUrgent; }
	[[nodiscard]] QBindable<bool> bindableCanActivate() const { return &this->bCanActivate; }
	[[nodiscard]] QBindable<bool> bindableCanDeactivate() const { return &this->bCanDeactivate; }
	[[nodiscard]] QBindable<bool> bindableCanRemove() const { return &this->bCanRemove; }
	[[nodiscard]] QBindable<bool> bindableCanSetGroup() const { return &this->bCanSetGroup; }

signals:
	void idChanged();
	void nameChanged();
	void activeChanged();
	void shouldDisplayChanged();
	void urgentChanged();
	void canActivateChanged();
	void canDeactivateChanged();
	void canRemoveChanged();
	void canSetGroupChanged();

protected:
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, QString, bId, &Workspace::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, QString, bName, &Workspace::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bActive, &Workspace::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bShouldDisplay, &Workspace::shouldDisplayChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bUrgent, &Workspace::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanActivate, &Workspace::canActivateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanDeactivate, &Workspace::canDeactivateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanRemove, &Workspace::canRemoveChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanSetGroup, &Workspace::canSetGroupChanged);
	//Q_OBJECT_BINDABLE_PROPERTY(Workspace, WorkspaceGroup*, bGroup);
	//Q_OBJECT_BINDABLE_PROPERTY(Workspace, qint32, bIndex);
};

} // namespace qs::wm
