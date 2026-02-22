#pragma once

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qevent.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qproperty.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qsurfaceformat.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qvectornd.h>
#include <qwindow.h>

#include "../core/qmlscreen.hpp"
#include "../core/region.hpp"
#include "../core/reload.hpp"
#include "windowinterface.hpp"

class ProxiedWindow;
class ProxyWindowContentItem;

// Proxy to an actual window exposing a limited property set with the ability to
// transfer it to a new window.

///! Base class for reloadable windows
///
/// [ShellWindow]: ../shellwindow
/// [FloatingWindow]: ../floatingwindow
class ProxyWindowBase: public Reloadable {
	Q_OBJECT;
	// clang-format off
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
	Q_PROPERTY(qint32 implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged);
	Q_PROPERTY(qint32 implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged);
	Q_PROPERTY(qint32 width READ width WRITE setWidth NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height WRITE setHeight NOTIFY heightChanged);
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged);
	Q_PROPERTY(QuickshellScreenInfo* screen READ screen WRITE setScreen NOTIFY screenChanged);
	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
	Q_PROPERTY(PendingRegion* mask READ mask WRITE setMask NOTIFY maskChanged);
	Q_PROPERTY(QObject* windowTransform READ windowTransform NOTIFY windowTransformChanged);
	Q_PROPERTY(bool backingWindowVisible READ isVisibleDirect NOTIFY backerVisibilityChanged);
	Q_PROPERTY(QsSurfaceFormat surfaceFormat READ surfaceFormat WRITE setSurfaceFormat NOTIFY surfaceFormatChanged);
	Q_PROPERTY(bool updatesEnabled READ updatesEnabled WRITE setUpdatesEnabled NOTIFY updatesEnabledChanged);
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	// clang-format on
	Q_CLASSINFO("DefaultProperty", "data");

public:
	explicit ProxyWindowBase(QObject* parent = nullptr);
	~ProxyWindowBase() override;

	ProxyWindowBase(ProxyWindowBase&) = delete;
	ProxyWindowBase(ProxyWindowBase&&) = delete;
	void operator=(ProxyWindowBase&) = delete;
	void operator=(ProxyWindowBase&&) = delete;

	Q_INVOKABLE [[nodiscard]] QPointF itemPosition(QQuickItem* item) const;
	Q_INVOKABLE [[nodiscard]] QRectF itemRect(QQuickItem* item) const;
	Q_INVOKABLE [[nodiscard]] QPointF mapFromItem(QQuickItem* item, QPointF point) const;
	Q_INVOKABLE [[nodiscard]] QPointF mapFromItem(QQuickItem* item, qreal x, qreal y) const;
	Q_INVOKABLE [[nodiscard]] QRectF mapFromItem(QQuickItem* item, QRectF rect) const;
	Q_INVOKABLE [[nodiscard]] QRectF
	mapFromItem(QQuickItem* item, qreal x, qreal y, qreal width, qreal height) const;

	void onReload(QObject* oldInstance) override;
	void ensureQWindow();
	void createWindow();
	void deleteWindow(bool keepItemOwnership = false);

	// Disown the backing window and delete all its children.
	virtual ProxiedWindow* disownWindow(bool keepItemOwnership = false);

	virtual ProxiedWindow* retrieveWindow(QObject* oldInstance);
	virtual ProxiedWindow* createQQuickWindow();
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

	[[nodiscard]] QBindable<bool> bindableBackerVisibility() const {
		return &this->bBackerVisibility;
	}

	void schedulePolish();

	[[nodiscard]] virtual qint32 x() const;
	[[nodiscard]] virtual qint32 y() const;

	[[nodiscard]] qint32 implicitWidth() const { return this->bImplicitWidth; }
	void setImplicitWidth(qint32 implicitWidth);

	[[nodiscard]] qint32 implicitHeight() const { return this->bImplicitHeight; }
	void setImplicitHeight(qint32 implicitHeight);

	[[nodiscard]] virtual qint32 width() const;
	void setWidth(qint32 width);

	[[nodiscard]] virtual qint32 height() const;
	void setHeight(qint32 height);

	virtual void trySetWidth(qint32 implicitWidth);
	virtual void trySetHeight(qint32 implicitHeight);

	[[nodiscard]] qreal devicePixelRatio() const;

	[[nodiscard]] QScreen* qscreen() const;
	[[nodiscard]] QuickshellScreenInfo* screen() const;
	virtual void setScreen(QuickshellScreenInfo* screen);

	[[nodiscard]] QColor color() const;
	virtual void setColor(QColor color);

	[[nodiscard]] PendingRegion* mask() const;
	virtual void setMask(PendingRegion* mask);

	[[nodiscard]] QsSurfaceFormat surfaceFormat() const { return this->qsSurfaceFormat; }
	void setSurfaceFormat(QsSurfaceFormat format);

	[[nodiscard]] bool updatesEnabled() const;
	void setUpdatesEnabled(bool updatesEnabled);

	[[nodiscard]] QObject* windowTransform() const { return nullptr; } // NOLINT

	[[nodiscard]] QQmlListProperty<QObject> data();

signals:
	void closed();
	void resourcesLost();
	void windowConnected();
	void windowDestroyed();
	void visibleChanged();
	void backerVisibilityChanged();
	void xChanged();
	void yChanged();
	void implicitWidthChanged();
	void implicitHeightChanged();
	void widthChanged();
	void heightChanged();
	void devicePixelRatioChanged();
	void windowTransformChanged();
	void screenChanged();
	void colorChanged();
	void maskChanged();
	void surfaceFormatChanged();
	void updatesEnabledChanged();
	void polished();

protected slots:
	virtual void onWidthChanged();
	virtual void onHeightChanged();
	virtual void onPolished();

private slots:
	void onSceneGraphError(QQuickWindow::SceneGraphError error, const QString& message);
	void onVisibleChanged();
	void onMaskChanged();
	void onMaskDestroyed();
	void onScreenDestroyed();
	void onExposed();

protected:
	bool mVisible = true;
	QScreen* mScreen = nullptr;
	QColor mColor = Qt::white;
	PendingRegion* mMask = nullptr;
	ProxiedWindow* window = nullptr;
	ProxyWindowContentItem* mContentItem = nullptr;
	bool reloadComplete = false;
	bool ranLints = false;
	bool mUpdatesEnabled = true;
	QsSurfaceFormat qsSurfaceFormat;
	QSurfaceFormat mSurfaceFormat;

	struct {
		bool inputMask : 1 = false;
	} pendingPolish;

	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(
	    ProxyWindowBase,
	    qint32,
	    bImplicitWidth,
	    100,
	    &ProxyWindowBase::implicitWidthChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(
	    ProxyWindowBase,
	    qint32,
	    bImplicitHeight,
	    100,
	    &ProxyWindowBase::implicitHeightChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    ProxyWindowBase,
	    bool,
	    bBackerVisibility,
	    &ProxyWindowBase::backerVisibilityChanged
	);

private:
	void polishItems();
	void updateMask();
};

class ProxyWindowAttached: public QsWindowAttached {
	Q_OBJECT;

public:
	explicit ProxyWindowAttached(QQuickItem* parent);

	[[nodiscard]] QObject* window() const override;
	[[nodiscard]] ProxyWindowBase* proxyWindow() const override;
	[[nodiscard]] QQuickItem* contentItem() const override;

protected:
	void updateWindow() override;

private:
	ProxyWindowBase* mWindow = nullptr;
	QObject* mWindowInterface = nullptr;

	void setWindow(ProxyWindowBase* window);
};

class ProxiedWindow: public QQuickWindow {
	Q_OBJECT;

public:
	explicit ProxiedWindow(ProxyWindowBase* proxy, QWindow* parent = nullptr)
	    : QQuickWindow(parent)
	    , mProxy(proxy) {}

	[[nodiscard]] ProxyWindowBase* proxy() const { return this->mProxy; }
	void setProxy(ProxyWindowBase* proxy) { this->mProxy = proxy; }

signals:
	void exposed();
	void devicePixelRatioChanged();

protected:
	bool event(QEvent* event) override;
	void exposeEvent(QExposeEvent* event) override;

private:
	ProxyWindowBase* mProxy;
};

class ProxyWindowContentItem: public QQuickItem {
	Q_OBJECT;

signals:
	void polished();

protected:
	void updatePolish() override;
};
