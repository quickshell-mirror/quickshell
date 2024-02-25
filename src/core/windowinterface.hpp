#pragma once

#include <qcolor.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlscreen.hpp"
#include "region.hpp"
#include "reload.hpp"

class WindowInterface: public Reloadable {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QQuickItem* contentItem READ contentItem);
	/// If the window is shown or hidden. Defaults to true.
	Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged);
	Q_PROPERTY(qint32 width READ width WRITE setWidth NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height WRITE setHeight NOTIFY heightChanged);
	/// The screen that the window currently occupies.
	///
	/// > [!INFO] This cannot be changed after windowConnected.
	Q_PROPERTY(QuickShellScreenInfo* screen READ screen WRITE setScreen NOTIFY screenChanged);
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
	Q_PROPERTY(QQmlListProperty<QObject> data READ data);
	// clang-format on
	Q_CLASSINFO("DefaultProperty", "data");
	QML_NAMED_ELEMENT(QSWindow);
	QML_UNCREATABLE("uncreatable base class");

public:
	explicit WindowInterface(QObject* parent = nullptr): Reloadable(parent) {}

	[[nodiscard]] virtual QQuickItem* contentItem() const = 0;

	[[nodiscard]] virtual bool isVisible() const = 0;
	virtual void setVisible(bool visible) = 0;

	[[nodiscard]] virtual qint32 width() const = 0;
	virtual void setWidth(qint32 width) = 0;

	[[nodiscard]] virtual qint32 height() const = 0;
	virtual void setHeight(qint32 height) = 0;

	[[nodiscard]] virtual QuickShellScreenInfo* screen() const = 0;
	virtual void setScreen(QuickShellScreenInfo* screen) = 0;

	[[nodiscard]] virtual QColor color() const = 0;
	virtual void setColor(QColor color) = 0;

	[[nodiscard]] virtual PendingRegion* mask() const = 0;
	virtual void setMask(PendingRegion* mask) = 0;

	[[nodiscard]] virtual QQmlListProperty<QObject> data() = 0;

signals:
	void windowConnected();
	void visibleChanged();
	void widthChanged();
	void heightChanged();
	void screenChanged();
	void colorChanged();
	void maskChanged();
};
