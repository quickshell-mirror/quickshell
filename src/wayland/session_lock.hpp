#pragma once

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qguiapplication.h>
#include <qmap.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/qmlscreen.hpp"
#include "../core/reload.hpp"
#include "session_lock/session_lock.hpp"

class WlSessionLockSurface;

///! Wayland session locker.
/// Wayland session lock implemented using the [ext_session_lock_v1] protocol.
///
/// WlSessionLock will create an instance of its `surface` component for every screen when
/// `locked` is set to true. The `surface` component must create a @@WlSessionLockSurface
/// which will be displayed on each screen.
///
/// The below example will create a session lock that disappears when the button is clicked.
/// ```qml
/// WlSessionLock {
///   id: lock
///
///   WlSessionLockSurface {
///     Button {
///       text: "unlock me"
///       onClicked: lock.locked = false
///     }
///   }
/// }
///
/// // ...
/// lock.locked = true
/// ```
///
/// > [!WARNING] If the WlSessionLock is destroyed or quickshell exits without setting `locked`
/// > to false, conformant compositors will leave the screen locked and painted with a solid
/// > color.
/// >
/// > This is what makes the session lock secure. The lock dying will not expose your session,
/// > but it will render it inoperable.
///
/// [ext_session_lock_v1]: https://wayland.app/protocols/ext-session-lock-v1
class WlSessionLock: public Reloadable {
	Q_OBJECT;
	// clang-format off
	/// Controls the lock state.
	///
	/// > [!WARNING] Only one WlSessionLock may be locked at a time. Attempting to enable a lock while
	/// > another lock is enabled will do nothing.
	Q_PROPERTY(bool locked READ isLocked WRITE setLocked NOTIFY lockStateChanged);
	/// The compositor lock state.
	///
	/// This is set to true once the compositor has confirmed all screens are covered with locks.
	Q_PROPERTY(bool secure READ isSecure NOTIFY secureStateChanged);
	/// The surface that will be created for each screen. Must create a @@WlSessionLockSurface$.
	Q_PROPERTY(QQmlComponent* surface READ surfaceComponent WRITE setSurfaceComponent NOTIFY surfaceComponentChanged);
	// clang-format on
	QML_ELEMENT;
	Q_CLASSINFO("DefaultProperty", "surface");

public:
	explicit WlSessionLock(QObject* parent = nullptr): Reloadable(parent) {}

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] bool isLocked() const;
	void setLocked(bool locked);

	[[nodiscard]] bool isSecure() const;

	[[nodiscard]] QQmlComponent* surfaceComponent() const;
	void setSurfaceComponent(QQmlComponent* surfaceComponent);

signals:
	void lockStateChanged();
	void secureStateChanged();
	void surfaceComponentChanged();

private slots:
	void unlock();
	void onScreensChanged();

private:
	void updateSurfaces(bool show, WlSessionLock* old = nullptr);
	void realizeLockTarget(WlSessionLock* old = nullptr);

	SessionLockManager* manager = nullptr;
	QQmlComponent* mSurfaceComponent = nullptr;
	QMap<QScreen*, WlSessionLockSurface*> surfaces;
	bool lockTarget = false;

	friend class WlSessionLockSurface;
};

///! Surface to display with a `WlSessionLock`.
/// Surface displayed by a @@WlSessionLock when it is locked.
class WlSessionLockSurface: public Reloadable {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QQuickItem* contentItem READ contentItem);
	/// If the surface has been made visible.
	///
	/// Note: SessionLockSurfaces will never become invisible, they will only be destroyed.
	Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged);
	Q_PROPERTY(qint32 width READ width NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height NOTIFY heightChanged);
	/// The screen that the surface is displayed on.
	Q_PROPERTY(QuickshellScreenInfo* screen READ screen NOTIFY screenChanged);
	/// The background color of the window. Defaults to white.
	///
	/// > [!WARNING] This seems to behave weirdly when using transparent colors on some systems.
	/// > Using a colored content item over a transparent window is the recommended way to work around this:
	/// > ```qml
	/// > ProxyWindow {
	/// >   Rectangle {
	/// >     anchors.fill: parent
	/// >     color: "#20ffffff"
	/// >
	/// >     // your content here
	/// >   }
	/// > }
	/// > ```
	/// > ... but you probably shouldn't make a transparent lock,
	/// > and most compositors will ignore an attempt to do so.
	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	// clang-format on
	QML_NAMED_ELEMENT(WlSessionLockSurface);
	Q_CLASSINFO("DefaultProperty", "data");

public:
	explicit WlSessionLockSurface(QObject* parent = nullptr);
	~WlSessionLockSurface() override;
	Q_DISABLE_COPY_MOVE(WlSessionLockSurface);

	void onReload(QObject* oldInstance) override;
	QQuickWindow* disownWindow();

	void attach();
	void show();

	[[nodiscard]] QQuickItem* contentItem() const;

	[[nodiscard]] bool isVisible() const;

	[[nodiscard]] qint32 width() const;
	[[nodiscard]] qint32 height() const;

	[[nodiscard]] QuickshellScreenInfo* screen() const;
	void setScreen(QScreen* qscreen);

	[[nodiscard]] QColor color() const;
	void setColor(QColor color);

	[[nodiscard]] QQmlListProperty<QObject> data();

signals:
	void visibleChanged();
	void widthChanged();
	void heightChanged();
	void screenChanged();
	void colorChanged();

private slots:
	void onScreenDestroyed();
	void onWidthChanged();
	void onHeightChanged();

private:
	QQuickWindow* window = nullptr;
	QQuickItem* mContentItem;
	QScreen* mScreen = nullptr;
	QColor mColor = Qt::white;
	LockWindowExtension* ext;
};
