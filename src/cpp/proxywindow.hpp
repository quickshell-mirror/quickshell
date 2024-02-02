#pragma once

#include <qcolor.h>
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
	Q_PROPERTY(QQuickItem* item READ item CONSTANT);
	Q_PROPERTY(bool visible READ isVisible WRITE setVisible);
	Q_PROPERTY(qint32 width READ width WRITE setWidth);
	Q_PROPERTY(qint32 height READ height WRITE setHeight);
	Q_PROPERTY(QColor color READ color WRITE setColor);
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	Q_CLASSINFO("DefaultProperty", "data");

protected:
	void earlyInit(QObject* old) override;

public:
	explicit ProxyWindowBase(QObject* parent = nullptr): Scavenger(parent) {}
	~ProxyWindowBase() override;

	ProxyWindowBase(ProxyWindowBase&) = delete;
	ProxyWindowBase(ProxyWindowBase&&) = delete;
	void operator=(ProxyWindowBase&) = delete;
	void operator=(ProxyWindowBase&&) = delete;

	// Disown the backing window and delete all its children.
	QQuickWindow* disownWindow();

	QQuickItem* item();

	bool isVisible();
	virtual void setVisible(bool value);

	qint32 width();
	virtual void setWidth(qint32 value);

	qint32 height();
	virtual void setHeight(qint32 value);

	QColor color();
	void setColor(QColor value);

	QQmlListProperty<QObject> data();

private:
	static QQmlListProperty<QObject> dataBacker(QQmlListProperty<QObject>* prop);
	static void dataAppend(QQmlListProperty<QObject>* prop, QObject* obj);
	static qsizetype dataCount(QQmlListProperty<QObject>* prop);
	static QObject* dataAt(QQmlListProperty<QObject>* prop, qsizetype i);
	static void dataClear(QQmlListProperty<QObject>* prop);
	static void dataReplace(QQmlListProperty<QObject>* prop, qsizetype i, QObject* obj);
	static void dataRemoveLast(QQmlListProperty<QObject>* prop);

	QQuickWindow* window = nullptr;
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
