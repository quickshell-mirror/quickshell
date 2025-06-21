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

class QuickshellScreenInfo;

namespace qs::wm {

Q_DECLARE_LOGGING_CATEGORY(logWorkspace);

class WorkspaceGroup;

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
	Q_PROPERTY(WorkspaceGroup* group READ default BINDABLE bindableGroup NOTIFY groupChanged);
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
	Q_INVOKABLE virtual void setGroup(WorkspaceGroup* group);

	[[nodiscard]] QBindable<QString> bindableId() const { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }
	[[nodiscard]] QBindable<bool> bindableActive() const { return &this->bActive; }
	[[nodiscard]] QBindable<WorkspaceGroup*> bindableGroup() const { return &this->bGroup; }
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
	void groupChanged();
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
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, WorkspaceGroup*, bGroup, &Workspace::groupChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bShouldDisplay, &Workspace::shouldDisplayChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bUrgent, &Workspace::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanActivate, &Workspace::canActivateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanDeactivate, &Workspace::canDeactivateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanRemove, &Workspace::canRemoveChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Workspace, bool, bCanSetGroup, &Workspace::canSetGroupChanged);
	//Q_OBJECT_BINDABLE_PROPERTY(Workspace, qint32, bIndex);
};

class WorkspaceGroup: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	/// Screens the workspace group is present on.
	///
	/// > [!WARNING] This is not a model. Use @@Quickshell.ScriptModel if you need it to
	/// > behave like one.
	Q_PROPERTY(QList<QuickshellScreenInfo*> screens READ screens NOTIFY screensChanged);

public:
	explicit WorkspaceGroup(QObject* parent): QObject(parent) {}

	[[nodiscard]] const QList<QuickshellScreenInfo*>& screens() const { return this->mCachedScreens; }

signals:
	void screensChanged();

private slots:
	void onScreensChanged();

protected:
	Q_OBJECT_BINDABLE_PROPERTY(
	    WorkspaceGroup,
	    QList<QScreen*>,
	    bScreens,
	    &WorkspaceGroup::onScreensChanged
	);

private:
	QList<QuickshellScreenInfo*> mCachedScreens;
};

} // namespace qs::wm
