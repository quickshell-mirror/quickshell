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

class SessionLockSurface;

/// Note: Very untested. Do anything outside of the obvious use cases and you WILL red screen.
class SessionLock: public Reloadable {
	Q_OBJECT;
	// clang-format off
	/// Note: only one SessionLock may be locked at a time.
	Q_PROPERTY(bool locked READ isLocked WRITE setLocked NOTIFY lockStateChanged);
	/// Returns the *compositor* lock state, which will only be set to true after all surfaces are in place.
	Q_PROPERTY(bool secure READ isSecure NOTIFY secureStateChanged);
	/// Component that will be instantiated for each screen. Must be a `SessionLockSurface`.
	Q_PROPERTY(QQmlComponent* surface READ surfaceComponent WRITE setSurfaceComponent NOTIFY surfaceComponentChanged);
	// clang-format on
	QML_ELEMENT;
	Q_CLASSINFO("DefaultProperty", "surface");

public:
	explicit SessionLock(QObject* parent = nullptr): Reloadable(parent) {}

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
	void updateSurfaces(SessionLock* old = nullptr);

	SessionLockManager* manager = nullptr;
	QQmlComponent* mSurfaceComponent = nullptr;
	QMap<QScreen*, SessionLockSurface*> surfaces;
	bool lockTarget = false;

	friend class SessionLockSurface;
};

class SessionLockSurface: public Reloadable {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QQuickItem* contentItem READ contentItem);
	/// If the window has been presented yet.
	Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged);
	Q_PROPERTY(qint32 width READ width NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height NOTIFY heightChanged);
	/// The screen that the window currently occupies.
	///
	/// > [!INFO] This cannot be changed after windowConnected.
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
	QML_ELEMENT;
	Q_CLASSINFO("DefaultProperty", "data");

public:
	explicit SessionLockSurface(QObject* parent = nullptr);
	~SessionLockSurface() override;
	Q_DISABLE_COPY_MOVE(SessionLockSurface);

	void onReload(QObject* oldInstance) override;
	QQuickWindow* disownWindow();

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
