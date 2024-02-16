#pragma once

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qevent.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "region.hpp"
#include "reload.hpp"

// Proxy to an actual window exposing a limited property set with the ability to
// transfer it to a new window.
// Detaching a window and touching any property is a use after free.
//
// NOTE: setting an `id` in qml will point to the proxy window and not the real window so things
// like anchors must use `item`.
class ProxyWindowBase: public Reloadable {
	Q_OBJECT;
	/// The QtQuick window backing this window.
	///
	/// > [!WARNING] Do not expect values set via this property to work correctly.
	/// > Values set this way will almost certainly misbehave across a reload, possibly
	/// > even without one.
	/// >
	/// > Use **only** if you know what you are doing.
	Q_PROPERTY(QQuickWindow* _backingWindow READ backingWindow);
	/// The visibility of the window.
	///
	/// > [!INFO] Windows are not visible by default so you will need to set this to make the window
	/// > appear.
	Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged);
	Q_PROPERTY(qint32 width READ width WRITE setWidth NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height WRITE setHeight NOTIFY heightChanged);
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
	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
	/// The clickthrough mask. Defaults to null.
	///
	/// If non null then the clickable areas of the window will be determined by the provided region.
	///
	/// ```qml
	/// ProxyShellWindow {
	///   // The mask region is set to `rect`, meaning only `rect` is clickable.
	///   // All other clicks pass through the window to ones behind it.
	///   mask: Region { item: rect }
	///
	///   Rectangle {
	///     id: rect
	///
	///     anchors.centerIn: parent
	///     width: 100
	///     height: 100
	///   }
	/// }
	/// ```
	///
	/// If the provided region's intersection mode is `Combine` (the default),
	/// then the region will be used as is. Otherwise it will be applied on top of the window region.
	///
	/// For example, setting the intersection mode to `Xor` will invert the mask and make everything in
	/// the mask region not clickable and pass through clicks inside it through the window.
	///
	/// ```qml
	/// ProxyShellWindow {
	///   // The mask region is set to `rect`, but the intersection mode is set to `Xor`.
	///   // This inverts the mask causing all clicks inside `rect` to be passed to the window
	///   // behind this one.
	///   mask: Region { item: rect; intersection: Intersection.Xor }
	///
	///   Rectangle {
	///     id: rect
	///
	///     anchors.centerIn: parent
	///     width: 100
	///     height: 100
	///   }
	/// }
	/// ```
	Q_PROPERTY(PendingRegion* mask READ mask WRITE setMask NOTIFY maskChanged);
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	Q_CLASSINFO("DefaultProperty", "data");

public:
	explicit ProxyWindowBase(QObject* parent = nullptr): Reloadable(parent) {}
	~ProxyWindowBase() override;

	ProxyWindowBase(ProxyWindowBase&) = delete;
	ProxyWindowBase(ProxyWindowBase&&) = delete;
	void operator=(ProxyWindowBase&) = delete;
	void operator=(ProxyWindowBase&&) = delete;

	void onReload(QObject* oldInstance) override;

	virtual void setupWindow();

	// Disown the backing window and delete all its children.
	virtual QQuickWindow* disownWindow();

	[[nodiscard]] QQuickWindow* backingWindow() const;

	[[nodiscard]] virtual bool isVisible() const;
	virtual void setVisible(bool visible);

	[[nodiscard]] virtual qint32 width() const;
	virtual void setWidth(qint32 width);

	[[nodiscard]] virtual qint32 height() const;
	virtual void setHeight(qint32 height);

	[[nodiscard]] QColor color() const;
	void setColor(QColor color);

	[[nodiscard]] PendingRegion* mask() const;
	void setMask(PendingRegion* mask);

	QQmlListProperty<QObject> data();

signals:
	void windowConnected();
	void visibleChanged();
	void widthChanged();
	void heightChanged();
	void colorChanged();
	void maskChanged();

private slots:
	void onMaskChanged();

protected:
	bool mVisible = false;
	qint32 mWidth = 100;
	qint32 mHeight = 100;
	QColor mColor;
	PendingRegion* mMask = nullptr;
	QQuickWindow* window = nullptr;

private:
	void updateMask();
	QQmlListProperty<QObject> dataBacker();

	static void dataAppend(QQmlListProperty<QObject>* prop, QObject* obj);
	static qsizetype dataCount(QQmlListProperty<QObject>* prop);
	static QObject* dataAt(QQmlListProperty<QObject>* prop, qsizetype i);
	static void dataClear(QQmlListProperty<QObject>* prop);
	static void dataReplace(QQmlListProperty<QObject>* prop, qsizetype i, QObject* obj);
	static void dataRemoveLast(QQmlListProperty<QObject>* prop);

	QVector<QObject*> pendingChildren;
};

// qt attempts to resize the window but fails because wayland
// and only resizes the graphics context which looks terrible.
class ProxyFloatingWindow: public ProxyWindowBase {
	Q_OBJECT;
	QML_ELEMENT;

public:
	// Setting geometry while the window is visible makes the content item shrink but not the window
	// which is awful so we disable it for floating windows.
	void setWidth(qint32 width) override;
	void setHeight(qint32 height) override;
};
