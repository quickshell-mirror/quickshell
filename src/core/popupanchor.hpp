#pragma once

#include <optional>

#include <qflags.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qsize.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qvectornd.h>
#include <qwindow.h>

#include "../window/proxywindow.hpp"
#include "doc.hpp"
#include "types.hpp"

///! Adjustment strategy for popups that do not fit on screen.
/// Adjustment strategy for popups. See @@PopupAnchor.adjustment.
///
/// Adjustment flags can be combined with the `|` operator.
///
/// `Flip` will be applied first, then `Slide`, then `Resize`.
namespace PopupAdjustment { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
	None = 0,
	/// If the X axis is constrained, the popup will slide along the X axis until it fits onscreen.
	SlideX = 1,
	/// If the Y axis is constrained, the popup will slide along the Y axis until it fits onscreen.
	SlideY = 2,
	/// Alias for `SlideX | SlideY`.
	Slide = SlideX | SlideY,
	/// If the X axis is constrained, the popup will invert its horizontal gravity if any.
	FlipX = 4,
	/// If the Y axis is constrained, the popup will invert its vertical gravity if any.
	FlipY = 8,
	/// Alias for `FlipX | FlipY`.
	Flip = FlipX | FlipY,
	/// If the X axis is constrained, the width of the popup will be reduced to fit on screen.
	ResizeX = 16,
	/// If the Y axis is constrained, the height of the popup will be reduced to fit on screen.
	ResizeY = 32,
	/// Alias for `ResizeX | ResizeY`
	Resize = ResizeX | ResizeY,
	/// Alias for `Flip | Slide | Resize`.
	All = Slide | Flip | Resize,
};
Q_ENUM_NS(Enum);
Q_DECLARE_FLAGS(Flags, Enum);

} // namespace PopupAdjustment

Q_DECLARE_OPERATORS_FOR_FLAGS(PopupAdjustment::Flags);

struct PopupAnchorState {
	bool operator==(const PopupAnchorState& other) const;

	QRect rect = {0, 0, 1, 1};
	Edges::Flags edges = Edges::Top | Edges::Left;
	Edges::Flags gravity = Edges::Bottom | Edges::Right;
	PopupAdjustment::Flags adjustment = PopupAdjustment::Slide;
	QPoint anchorpoint;
	QSize size;
};

///! Anchorpoint or positioner for popup windows.
class PopupAnchor: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The window to anchor / attach the popup to. Setting this property unsets @@item.
	Q_PROPERTY(QObject* window READ window WRITE setWindow NOTIFY windowChanged);
	/// The item to anchor / attach the popup to. Setting this property unsets @@window.
	///
	/// The popup's position relative to its parent window is only calculated when it is
	/// initially shown (directly before @@anchoring(s) is emitted), meaning its anchor
	/// rectangle will be set relative to the item's position in the window at that time.
	/// @@updateAnchor() can be called to update the anchor rectangle if the item's position
	/// has changed.
	///
	/// > [!NOTE] If a more flexible way to position a popup relative to an item is needed,
	/// > set @@window to the item's parent window, and handle the @@anchoring signal to
	/// > position the popup relative to the window's contentItem.
	Q_PROPERTY(QQuickItem* item READ item WRITE setItem NOTIFY itemChanged);
	/// The anchorpoints the popup will attach to, relative to @@item or @@window.
	/// Which anchors will be used is determined by the @@edges, @@gravity, and @@adjustment.
	///
	/// If using @@item, the default anchor rectangle matches the dimensions of the item.
	///
	/// If you leave @@edges, @@gravity and @@adjustment at their default values,
	/// setting more than `x` and `y` does not matter. The anchor rect cannot
	/// be smaller than 1x1 pixels.
	///
	/// [coordinate mapping functions]: https://doc.qt.io/qt-6/qml-qtquick-item.html#mapFromItem-method
	Q_PROPERTY(Box rect READ rect WRITE setRect RESET resetRect NOTIFY rectChanged);
	/// A margin applied to the anchor rect.
	///
	/// This is most useful when @@item is used and @@rect is left at its default
	/// value (matching the Item's dimensions).
	Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged);
	/// The point on the anchor rectangle the popup should anchor to.
	/// Opposing edges suchs as `Edges.Left | Edges.Right` are not allowed.
	///
	/// Defaults to `Edges.Top | Edges.Left`.
	Q_PROPERTY(Edges::Flags edges READ edges WRITE setEdges NOTIFY edgesChanged);
	/// The direction the popup should expand towards, relative to the anchorpoint.
	/// Opposing edges suchs as `Edges.Left | Edges.Right` are not allowed.
	///
	/// Defaults to `Edges.Bottom | Edges.Right`.
	Q_PROPERTY(Edges::Flags gravity READ gravity WRITE setGravity NOTIFY gravityChanged);
	/// The strategy used to adjust the popup's position if it would otherwise not fit on screen,
	/// based on the anchor @@rect, preferred @@edges, and @@gravity.
	///
	/// See the documentation for @@PopupAdjustment for details.
	Q_PROPERTY(PopupAdjustment::Flags adjustment READ adjustment WRITE setAdjustment NOTIFY adjustmentChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("");

public:
	explicit PopupAnchor(QObject* parent): QObject(parent) {}

	/// Update the popup's anchor rect relative to its parent window.
	///
	/// If anchored to an item, popups anchors will not automatically follow
	/// the item if its position changes. This function can be called to
	/// recalculate the anchors.
	Q_INVOKABLE void updateAnchor();

	[[nodiscard]] bool isDirty() const;
	void markClean();
	void markDirty();

	[[nodiscard]] QObject* window() const { return this->mWindow; }
	[[nodiscard]] ProxyWindowBase* proxyWindow() const { return this->mProxyWindow; }
	[[nodiscard]] QWindow* backingWindow() const;
	void setWindowInternal(QObject* window);
	void setWindow(QObject* window);

	[[nodiscard]] QQuickItem* item() const { return this->mItem; }
	void setItem(QQuickItem* item);

	[[nodiscard]] QRect windowRect() const { return this->state.rect; }
	void setWindowRect(QRect rect);

	[[nodiscard]] Box rect() const { return this->mUserRect; }
	void setRect(Box rect);
	void resetRect();

	[[nodiscard]] Margins margins() const { return this->mMargins; }
	void setMargins(Margins margins);

	[[nodiscard]] Edges::Flags edges() const { return this->state.edges; }
	void setEdges(Edges::Flags edges);

	[[nodiscard]] Edges::Flags gravity() const { return this->state.gravity; }
	void setGravity(Edges::Flags gravity);

	[[nodiscard]] PopupAdjustment::Flags adjustment() const { return this->state.adjustment; }
	void setAdjustment(PopupAdjustment::Flags adjustment);

	void updatePlacement(const QPoint& anchorpoint, const QSize& size);

signals:
	/// Emitted when this anchor is about to be used. Mostly useful for modifying
	/// the anchor @@rect using [coordinate mapping functions], which are not reactive.
	///
	/// [coordinate mapping functions]: https://doc.qt.io/qt-6/qml-qtquick-item.html#mapFromItem-method
	void anchoring();

	void windowChanged();
	void itemChanged();
	QSDOC_HIDE void backingWindowVisibilityChanged();
	QSDOC_HIDE void windowRectChanged();
	void rectChanged();
	void marginsChanged();
	void edgesChanged();
	void gravityChanged();
	void adjustmentChanged();

private slots:
	void onWindowDestroyed();
	void onItemDestroyed();
	void onItemWindowChanged();

private:
	QObject* mWindow = nullptr;
	QQuickItem* mItem = nullptr;
	ProxyWindowBase* mProxyWindow = nullptr;
	PopupAnchorState state;
	Box mUserRect;
	Margins mMargins;
	std::optional<PopupAnchorState> lastState;
};

class PopupPositioner {
public:
	explicit PopupPositioner() = default;
	virtual ~PopupPositioner() = default;
	Q_DISABLE_COPY_MOVE(PopupPositioner);

	virtual void reposition(PopupAnchor* anchor, QWindow* window, bool onlyIfDirty = true);
	[[nodiscard]] virtual bool shouldRepositionOnMove() const;

	static PopupPositioner* instance();
	static void setInstance(PopupPositioner* instance);
};
