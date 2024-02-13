#pragma once

#include <qcolor.h>
#include <qevent.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "scavenge.hpp"

// Proxy to an actual window exposing a limited property set with the ability to
// transfer it to a new window.
// Detaching a window and touching any property is a use after free.
//
// NOTE: setting an `id` in qml will point to the proxy window and not the real window so things
// like anchors must use `item`.
class ProxyWindowBase: public Scavenger {
	Q_OBJECT;
	/// The content item of the window.
	Q_PROPERTY(QQuickItem* item READ item CONSTANT);
	/// The visibility of the window.
	///
	/// > [!INFO] Windows are not visible by default so you will need to set this to make the window
	/// appear.
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
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	Q_CLASSINFO("DefaultProperty", "data");

protected:
	void earlyInit(QObject* old) override;
	QQuickWindow* window = nullptr;

public:
	explicit ProxyWindowBase(QObject* parent = nullptr): Scavenger(parent) {}
	~ProxyWindowBase() override;

	ProxyWindowBase(ProxyWindowBase&) = delete;
	ProxyWindowBase(ProxyWindowBase&&) = delete;
	void operator=(ProxyWindowBase&) = delete;
	void operator=(ProxyWindowBase&&) = delete;

	// Disown the backing window and delete all its children.
	virtual QQuickWindow* disownWindow();

	QQuickItem* item();

	virtual bool isVisible();
	virtual void setVisible(bool value);

	virtual qint32 width();
	virtual void setWidth(qint32 value);

	virtual qint32 height();
	virtual void setHeight(qint32 value);

	QColor color();
	void setColor(QColor value);

	QQmlListProperty<QObject> data();

signals:
	void visibleChanged(bool visible);
	void widthChanged(qint32 width);
	void heightChanged(qint32 width);
	void colorChanged(QColor color);

private:
	static QQmlListProperty<QObject> dataBacker(QQmlListProperty<QObject>* prop);
	static void dataAppend(QQmlListProperty<QObject>* prop, QObject* obj);
	static qsizetype dataCount(QQmlListProperty<QObject>* prop);
	static QObject* dataAt(QQmlListProperty<QObject>* prop, qsizetype i);
	static void dataClear(QQmlListProperty<QObject>* prop);
	static void dataReplace(QQmlListProperty<QObject>* prop, qsizetype i, QObject* obj);
	static void dataRemoveLast(QQmlListProperty<QObject>* prop);
};

// qt attempts to resize the window but fails because wayland
// and only resizes the graphics context which looks terrible.
class ProxyFloatingWindow: public ProxyWindowBase {
	Q_OBJECT;
	QML_ELEMENT;

public:
	void earlyInit(QObject* old) override;
	void componentComplete() override;

	void setWidth(qint32 value) override;
	void setHeight(qint32 value) override;

private:
	bool geometryLocked = false;
};
