#pragma once

#include <qcolor.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/qmlscreen.hpp"
#include "../core/region.hpp"
#include "../core/reload.hpp"

class ProxyWindowBase;
class QsWindowAttached;

class QsSurfaceFormat {
	Q_GADGET;
	QML_VALUE_TYPE(surfaceFormat);
	QML_STRUCTURED_VALUE;
	Q_PROPERTY(bool opaque MEMBER opaque WRITE setOpaque);

public:
	bool opaque = false;
	bool opaqueModified = false;

	void setOpaque(bool opaque) {
		this->opaque = opaque;
		this->opaqueModified = true;
	}

	[[nodiscard]] bool operator==(const QsSurfaceFormat& other) const {
		return other.opaqueModified == this->opaqueModified && other.opaque == this->opaque;
	}
};

///! Base class of Quickshell windows
/// Base class of Quickshell windows
/// ### Attached properties
/// `QSWindow` can be used as an attached object of anything that subclasses @@QtQuick.Item$.
/// It provides the following properties
/// - `window` - the `QSWindow` object.
/// - `contentItem` - the `contentItem` property of the window.
///
/// @@itemPosition(), @@itemRect(), and @@mapFromItem() can also be called directly
/// on the attached object.
class WindowInterface: public Reloadable {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT);
	/// If the window should be shown or hidden. Defaults to true.
	Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged);
	/// If the window is currently shown. You should generally prefer [visible](#prop.visible).
	///
	/// This property is useful for ensuring windows spawn in a specific order, and you should
	/// not use it in place of [visible](#prop.visible).
	Q_PROPERTY(bool backingWindowVisible READ isBackingWindowVisible NOTIFY backingWindowVisibleChanged);
	/// The window's desired width.
	Q_PROPERTY(qint32 implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged);
	/// The window's desired height.
	Q_PROPERTY(qint32 implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged);
	/// The window's actual width.
	///
	/// Setting this property is deprecated. Set @@implicitWidth instead.
	Q_PROPERTY(qint32 width READ width WRITE setWidth NOTIFY widthChanged);
	/// The window's actual height.
	///
	/// Setting this property is deprecated. Set @@implicitHeight instead.
	Q_PROPERTY(qint32 height READ height WRITE setHeight NOTIFY heightChanged);
	/// The ratio between logical pixels and monitor pixels.
	///
	/// Qt's coordinate system works in logical pixels, which equal N monitor pixels
	/// depending on scale factor. This property returns the amount of monitor pixels
	/// in a logical pixel for the current window.
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged);
	/// The screen that the window currently occupies.
	///
	/// This may be modified to move the window to the given screen.
	Q_PROPERTY(QuickshellScreenInfo* screen READ screen WRITE setScreen NOTIFY screenChanged);
	/// Opaque property that will receive an update when factors that affect the window's position
	/// and transform changed.
	///
	/// This property is intended to be used to force a binding update,
	/// along with map[To|From]Item (which is not reactive).
	Q_PROPERTY(QObject* windowTransform READ windowTransform NOTIFY windowTransformChanged);
	/// The background color of the window. Defaults to white.
	///
	/// > [!WARNING] If the window color is opaque before it is made visible,
	/// > it will not be able to become transparent later unless @@surfaceFormat$.opaque
	/// > is false.
	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
	/// The clickthrough mask. Defaults to null.
	///
	/// If non null then the clickable areas of the window will be determined by the provided region.
	///
	/// ```qml
	/// ShellWindow {
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
	/// ShellWindow {
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
	/// Set the surface format to request from the system.
	///
	/// - `opaque` - If the requested surface should be opaque. Opaque windows allow
	/// the operating system to avoid drawing things behind them, or blending the window
	/// with those behind it, saving power and GPU load. If unset, this property defaults to
	/// true if @@color is opaque, or false if not. *You should not need to modify this
	/// property unless you create a surface that starts opaque and later becomes transparent.*
	///
	/// > [!NOTE] The surface format cannot be changed after the window is created.
	Q_PROPERTY(QsSurfaceFormat surfaceFormat READ surfaceFormat WRITE setSurfaceFormat NOTIFY surfaceFormatChanged);
	/// If the window should receive render updates. Defaults to true.
	///
	/// When set to false, the window will not re-render in response to animations
	/// or other visual updates from other windows. This is useful for static windows
	/// such as wallpapers that do not need to update frequently, saving GPU cycles.
	Q_PROPERTY(bool updatesEnabled READ updatesEnabled WRITE setUpdatesEnabled NOTIFY updatesEnabledChanged);
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	// clang-format on
	Q_CLASSINFO("DefaultProperty", "data");
	QML_NAMED_ELEMENT(QsWindow);
	QML_UNCREATABLE("uncreatable base class");
	QML_ATTACHED(QsWindowAttached);

public:
	explicit WindowInterface(QObject* parent = nullptr): Reloadable(parent) {}

	/// Returns the given Item's position relative to the window. Does not update reactively.
	///
	/// Equivalent to calling `window.contentItem.mapFromItem(item, 0, 0)`
	///
	/// See also: @@QtQuick.Item.mapFromItem()
	Q_INVOKABLE [[nodiscard]] QPointF itemPosition(QQuickItem* item) const;
	/// Returns the given Item's geometry relative to the window. Does not update reactively.
	///
	/// Equivalent to calling `window.contentItem.mapFromItem(item, 0, 0, 0, 0)`
	///
	/// See also: @@QtQuick.Item.mapFromItem()
	Q_INVOKABLE [[nodiscard]] QRectF itemRect(QQuickItem* item) const;
	/// Maps the given point in the coordinate space of `item` to one in the coordinate space
	/// of this window. Does not update reactively.
	///
	/// Equivalent to calling `window.contentItem.mapFromItem(item, point)`
	///
	/// See also: @@QtQuick.Item.mapFromItem()
	Q_INVOKABLE [[nodiscard]] QPointF mapFromItem(QQuickItem* item, QPointF point) const;
	/// Maps the given point in the coordinate space of `item` to one in the coordinate space
	/// of this window. Does not update reactively.
	///
	/// Equivalent to calling `window.contentItem.mapFromItem(item, x, y)`
	///
	/// See also: @@QtQuick.Item.mapFromItem()
	Q_INVOKABLE [[nodiscard]] QPointF mapFromItem(QQuickItem* item, qreal x, qreal y) const;
	/// Maps the given rect in the coordinate space of `item` to one in the coordinate space
	/// of this window. Does not update reactively.
	///
	/// Equivalent to calling `window.contentItem.mapFromItem(item, rect)`
	///
	/// See also: @@QtQuick.Item.mapFromItem()
	Q_INVOKABLE [[nodiscard]] QRectF mapFromItem(QQuickItem* item, QRectF rect) const;
	// clang-format off
	/// Maps the given rect in the coordinate space of `item` to one in the coordinate space
	/// of this window. Does not update reactively.
	///
	/// Equivalent to calling `window.contentItem.mapFromItem(item, x, y, width, height)`
	///
	/// See also: @@QtQuick.Item.mapFromItem()
	Q_INVOKABLE [[nodiscard]] QRectF mapFromItem(QQuickItem* item, qreal x, qreal y, qreal width, qreal height) const;
	// clang-format on

	[[nodiscard]] virtual ProxyWindowBase* proxyWindow() const = 0;
	[[nodiscard]] QQuickItem* contentItem() const;

	[[nodiscard]] bool isVisible() const;
	[[nodiscard]] bool isBackingWindowVisible() const;
	void setVisible(bool visible) const;

	[[nodiscard]] qint32 implicitWidth() const;
	void setImplicitWidth(qint32 implicitWidth) const;

	[[nodiscard]] qint32 implicitHeight() const;
	void setImplicitHeight(qint32 implicitHeight) const;

	[[nodiscard]] qint32 width() const;
	void setWidth(qint32 width) const;

	[[nodiscard]] qint32 height() const;
	void setHeight(qint32 height) const;

	[[nodiscard]] qreal devicePixelRatio() const;

	[[nodiscard]] QuickshellScreenInfo* screen() const;
	void setScreen(QuickshellScreenInfo* screen) const;

	[[nodiscard]] QObject* windowTransform() const { return nullptr; } // NOLINT

	[[nodiscard]] QColor color() const;
	void setColor(QColor color) const;

	[[nodiscard]] PendingRegion* mask() const;
	void setMask(PendingRegion* mask) const;

	[[nodiscard]] QsSurfaceFormat surfaceFormat() const;
	void setSurfaceFormat(QsSurfaceFormat format) const;

	[[nodiscard]] bool updatesEnabled() const;
	void setUpdatesEnabled(bool updatesEnabled) const;

	[[nodiscard]] QQmlListProperty<QObject> data() const;

	static QsWindowAttached* qmlAttachedProperties(QObject* object);

signals:
	/// This signal is emitted when the window is closed by the user, the display server,
	/// or an error. It is not emitted when @@visible is set to false.
	void closed();
	/// This signal is emitted when resources a window depends on to display are lost,
	/// or could not be acquired during window creation. The most common trigger for
	/// this signal is a lack of VRAM when creating or resizing a window.
	///
	/// Following this signal, @@closed(s) will be sent.
	void resourcesLost();
	void windowConnected();
	void visibleChanged();
	void backingWindowVisibleChanged();
	void implicitWidthChanged();
	void implicitHeightChanged();
	void widthChanged();
	void heightChanged();
	void devicePixelRatioChanged();
	void screenChanged();
	void windowTransformChanged();
	void colorChanged();
	void maskChanged();
	void surfaceFormatChanged();
	void updatesEnabledChanged();

protected:
	void connectSignals() const;
};

class QsWindowAttached: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QObject* window READ window NOTIFY windowChanged);
	Q_PROPERTY(QQuickItem* contentItem READ contentItem NOTIFY windowChanged);
	QML_ANONYMOUS;

public:
	[[nodiscard]] virtual QObject* window() const = 0;
	[[nodiscard]] virtual ProxyWindowBase* proxyWindow() const = 0;
	[[nodiscard]] virtual QQuickItem* contentItem() const = 0;

	Q_INVOKABLE [[nodiscard]] QPointF itemPosition(QQuickItem* item) const;
	Q_INVOKABLE [[nodiscard]] QRectF itemRect(QQuickItem* item) const;
	Q_INVOKABLE [[nodiscard]] QPointF mapFromItem(QQuickItem* item, QPointF point) const;
	Q_INVOKABLE [[nodiscard]] QPointF mapFromItem(QQuickItem* item, qreal x, qreal y) const;
	Q_INVOKABLE [[nodiscard]] QRectF mapFromItem(QQuickItem* item, QRectF rect) const;

	Q_INVOKABLE [[nodiscard]] QRectF
	mapFromItem(QQuickItem* item, qreal x, qreal y, qreal width, qreal height) const;

signals:
	void windowChanged();

protected slots:
	virtual void updateWindow() = 0;

protected:
	explicit QsWindowAttached(QQuickItem* parent);
};
