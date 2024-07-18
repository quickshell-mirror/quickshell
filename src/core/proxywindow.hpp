#pragma once

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qevent.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlglobal.hpp"
#include "qmlscreen.hpp"
#include "region.hpp"
#include "reload.hpp"
#include "windowinterface.hpp"

// Proxy to an actual window exposing a limited property set with the ability to
// transfer it to a new window.

///! Base class for reloadable windows
///
/// [ShellWindow]: ../shellwindow
/// [FloatingWindow]: ../floatingwindow
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
	Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT);
	Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged);
	Q_PROPERTY(qint32 width READ width WRITE setWidth NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height WRITE setHeight NOTIFY heightChanged);
	Q_PROPERTY(QuickshellScreenInfo* screen READ screen WRITE setScreen NOTIFY screenChanged);
	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
	Q_PROPERTY(PendingRegion* mask READ mask WRITE setMask NOTIFY maskChanged);
	Q_PROPERTY(QObject* windowTransform READ windowTransform NOTIFY windowTransformChanged);
	Q_PROPERTY(bool backingWindowVisible READ isVisibleDirect NOTIFY backerVisibilityChanged);
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	Q_CLASSINFO("DefaultProperty", "data");

public:
	explicit ProxyWindowBase(QObject* parent = nullptr);
	~ProxyWindowBase() override;

	ProxyWindowBase(ProxyWindowBase&) = delete;
	ProxyWindowBase(ProxyWindowBase&&) = delete;
	void operator=(ProxyWindowBase&) = delete;
	void operator=(ProxyWindowBase&&) = delete;

	void onReload(QObject* oldInstance) override;
	void createWindow();
	void deleteWindow();

	// Disown the backing window and delete all its children.
	virtual QQuickWindow* disownWindow();

	virtual QQuickWindow* retrieveWindow(QObject* oldInstance);
	virtual QQuickWindow* createQQuickWindow();
	virtual void connectWindow();
	virtual void completeWindow();
	virtual void postCompleteWindow();
	[[nodiscard]] virtual bool deleteOnInvisible() const;

	[[nodiscard]] QQuickWindow* backingWindow() const;
	[[nodiscard]] QQuickItem* contentItem() const;

	[[nodiscard]] virtual bool isVisible() const;
	[[nodiscard]] virtual bool isVisibleDirect() const;
	virtual void setVisible(bool visible);
	virtual void setVisibleDirect(bool visible);

	[[nodiscard]] virtual qint32 x() const;
	[[nodiscard]] virtual qint32 y() const;

	[[nodiscard]] virtual qint32 width() const;
	virtual void setWidth(qint32 width);

	[[nodiscard]] virtual qint32 height() const;
	virtual void setHeight(qint32 height);

	[[nodiscard]] virtual QuickshellScreenInfo* screen() const;
	virtual void setScreen(QuickshellScreenInfo* screen);

	[[nodiscard]] QColor color() const;
	virtual void setColor(QColor color);

	[[nodiscard]] PendingRegion* mask() const;
	virtual void setMask(PendingRegion* mask);

	[[nodiscard]] QObject* windowTransform() const { return nullptr; } // NOLINT

	[[nodiscard]] QQmlListProperty<QObject> data();

signals:
	void windowConnected();
	void windowDestroyed();
	void visibleChanged();
	void backerVisibilityChanged();
	void xChanged();
	void yChanged();
	void widthChanged();
	void heightChanged();
	void windowTransformChanged();
	void screenChanged();
	void colorChanged();
	void maskChanged();

protected slots:
	virtual void onWidthChanged();
	virtual void onHeightChanged();
	void onMaskChanged();
	void onMaskDestroyed();
	void onScreenDestroyed();

protected:
	bool mVisible = true;
	qint32 mWidth = 100;
	qint32 mHeight = 100;
	QScreen* mScreen = nullptr;
	QColor mColor = Qt::white;
	PendingRegion* mMask = nullptr;
	QQuickWindow* window = nullptr;
	QQuickItem* mContentItem = nullptr;
	bool reloadComplete = false;

private:
	void polishItems();
	void updateMask();
};

class ProxyWindowAttached: public QsWindowAttached {
	Q_OBJECT;

public:
	explicit ProxyWindowAttached(ProxyWindowBase* window)
	    : QsWindowAttached(window)
	    , mWindow(window) {}

	[[nodiscard]] QObject* window() const override;
	[[nodiscard]] QQuickItem* contentItem() const override;

private:
	ProxyWindowBase* mWindow;
};
