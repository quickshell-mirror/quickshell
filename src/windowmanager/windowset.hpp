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

class WindowsetProjection;

///! A group of windows worked with by a user, usually known as a Workspace or Tag.
/// A Windowset is a generic type that encompasses both "Workspaces" and "Tags" in window managers.
/// Because the definition encompasses both you may not necessarily need all features.
class Windowset: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	/// A persistent internal identifier for the windowset. This property should be identical
	/// across restarts and destruction/recreation of a windowset.
	Q_PROPERTY(QString id READ default NOTIFY idChanged BINDABLE bindableId);
	/// Human readable name of the windowset.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// Coordinates of the workspace, represented as an N-dimensional array. Most WMs
	/// will only expose one coordinate. If more than one is exposed, the first is
	/// conventionally X, the second Y, and the third Z.
	Q_PROPERTY(QList<qint32> coordinates READ default NOTIFY coordinatesChanged BINDABLE bindableCoordinates);
	/// True if the windowset is currently active. In a workspace based WM, this means the
  /// represented workspace is current. In a tag based WM, this means the represented tag
	/// is active.
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	/// The projection this windowset is a member of. A projection is the set of screens covered by
	/// a windowset.
	Q_PROPERTY(WindowsetProjection* projection READ default NOTIFY projectionChanged BINDABLE bindableProjection);
	/// If false, this windowset should generally be hidden from workspace pickers.
	Q_PROPERTY(bool shouldDisplay READ default NOTIFY shouldDisplayChanged BINDABLE bindableShouldDisplay);
	/// If true, a window in this windowset has been marked as urgent.
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);
	/// If true, the windowset can be activated. In a workspace based WM, this will make the workspace
	/// current, in a tag based wm, the tag will be activated.
	Q_PROPERTY(bool canActivate READ default NOTIFY canActivateChanged BINDABLE bindableCanActivate);
	/// If true, the windowset can be deactivated. In a workspace based WM, deactivation is usually implicit
	/// and based on activation of another workspace.
	Q_PROPERTY(bool canDeactivate READ default NOTIFY canDeactivateChanged BINDABLE bindableCanDeactivate);
	/// If true, the windowset can be removed. This may be done implicitly by the WM as well.
	Q_PROPERTY(bool canRemove READ default NOTIFY canRemoveChanged BINDABLE bindableCanRemove);
	/// If true, the windowset can be moved to a different projection.
	Q_PROPERTY(bool canSetProjection READ default NOTIFY canSetProjectionChanged BINDABLE bindableCanSetProjection);
	// clang-format on

public:
	explicit Windowset(QObject* parent): QObject(parent) {}

	/// Activate the windowset, making it the current workspace on a workspace based WM, or activating
	/// the tag on a tag based WM. Requires @@canActivate.
	Q_INVOKABLE virtual void activate();
	/// Deactivate the windowset, hiding it. Requires @@canDeactivate.
	Q_INVOKABLE virtual void deactivate();
	/// Remove or destroy the windowset. Requires @@canRemove.
	Q_INVOKABLE virtual void remove();
	/// Move the windowset to a different projection. A projection represents the set of screens
	/// a workspace spans. Requires @@canSetProjection.
	Q_INVOKABLE virtual void setProjection(WindowsetProjection* projection);

	[[nodiscard]] QBindable<QString> bindableId() const { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }
	[[nodiscard]] QBindable<QList<qint32>> bindableCoordinates() const { return &this->bCoordinates; }
	[[nodiscard]] QBindable<bool> bindableActive() const { return &this->bActive; }

	[[nodiscard]] QBindable<WindowsetProjection*> bindableProjection() const {
		return &this->bProjection;
	}

	[[nodiscard]] QBindable<bool> bindableShouldDisplay() const { return &this->bShouldDisplay; }
	[[nodiscard]] QBindable<bool> bindableUrgent() const { return &this->bUrgent; }
	[[nodiscard]] QBindable<bool> bindableCanActivate() const { return &this->bCanActivate; }
	[[nodiscard]] QBindable<bool> bindableCanDeactivate() const { return &this->bCanDeactivate; }
	[[nodiscard]] QBindable<bool> bindableCanRemove() const { return &this->bCanRemove; }

	[[nodiscard]] QBindable<bool> bindableCanSetProjection() const {
		return &this->bCanSetProjection;
	}

signals:
	void idChanged();
	void nameChanged();
	void coordinatesChanged();
	void activeChanged();
	void projectionChanged();
	void shouldDisplayChanged();
	void urgentChanged();
	void canActivateChanged();
	void canDeactivateChanged();
	void canRemoveChanged();
	void canSetProjectionChanged();

protected:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, QString, bId, &Windowset::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, QString, bName, &Windowset::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, QList<qint32>, bCoordinates);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bActive, &Windowset::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, WindowsetProjection*, bProjection, &Windowset::projectionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bShouldDisplay, &Windowset::shouldDisplayChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bUrgent, &Windowset::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bCanActivate, &Windowset::canActivateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bCanDeactivate, &Windowset::canDeactivateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bCanRemove, &Windowset::canRemoveChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Windowset, bool, bCanSetProjection, &Windowset::canSetProjectionChanged);
	// clang-format on
};

///! A space occupiable by a Windowset.
/// A WindowsetProjection represents a space that can be occupied by one or more @@Windowset$s.
/// The space is one or more screens. Multiple projections may occupy the same screens.
///
/// @@WindowManager.screenProjection() can be used to get a projection representing all
/// @@Windowset$s on a given screen regardless of the WM's actual projection layout.
class WindowsetProjection: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");
	// clang-format off
	/// Screens the windowset projection spans, often a single screen or all screens.
	Q_PROPERTY(QList<QuickshellScreenInfo*> screens READ default NOTIFY screensChanged BINDABLE bindableScreens);
	/// Windowsets that are currently present on the projection.
	Q_PROPERTY(QList<Windowset*> windowsets READ default NOTIFY windowsetsChanged BINDABLE bindableWindowsets);
	// clang-format on

public:
	explicit WindowsetProjection(QObject* parent);

	[[nodiscard]] QBindable<QList<QuickshellScreenInfo*>> bindableScreens() const {
		return &this->bScreens;
	}

	[[nodiscard]] QBindable<QList<QScreen*>> bindableQScreens() const { return &this->bQScreens; }

	[[nodiscard]] QBindable<QList<Windowset*>> bindableWindowsets() const {
		return &this->bWindowsets;
	}

signals:
	void screensChanged();
	void windowsetsChanged();

protected:
	Q_OBJECT_BINDABLE_PROPERTY(WindowsetProjection, QList<QScreen*>, bQScreens);

	Q_OBJECT_BINDABLE_PROPERTY(
	    WindowsetProjection,
	    QList<QuickshellScreenInfo*>,
	    bScreens,
	    &WindowsetProjection::screensChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    WindowsetProjection,
	    QList<Windowset*>,
	    bWindowsets,
	    &WindowsetProjection::windowsetsChanged
	);
};

} // namespace qs::wm
