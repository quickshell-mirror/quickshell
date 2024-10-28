#pragma once

#include <optional>

#include <QtQmlIntegration/qqmlintegration.h>
#include <qflags.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmlintegration.h>
#include <qsize.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
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

enum Enum {
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

	Box rect = {0, 0, 1, 1};
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
	/// The window to anchor / attach the popup to.
	Q_PROPERTY(QObject* window READ window WRITE setWindow NOTIFY windowChanged);
	/// The anchorpoints the popup will attach to. Which anchors will be used is
	/// determined by the @@edges, @@gravity, and @@adjustment.
	///
	/// If you leave @@edges, @@gravity and @@adjustment at their default values,
	/// setting more than `x` and `y` does not matter.
	///
	/// > [!INFO] The anchor rect cannot be smaller than 1x1 pixels.
	Q_PROPERTY(Box rect READ rect WRITE setRect NOTIFY rectChanged);
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

	[[nodiscard]] bool isDirty() const;
	void markClean();
	void markDirty();

	[[nodiscard]] QObject* window() const;
	[[nodiscard]] ProxyWindowBase* proxyWindow() const;
	[[nodiscard]] QWindow* backingWindow() const;
	void setWindow(QObject* window);

	[[nodiscard]] Box rect() const;
	void setRect(Box rect);

	[[nodiscard]] Edges::Flags edges() const;
	void setEdges(Edges::Flags edges);

	[[nodiscard]] Edges::Flags gravity() const;
	void setGravity(Edges::Flags gravity);

	[[nodiscard]] PopupAdjustment::Flags adjustment() const;
	void setAdjustment(PopupAdjustment::Flags adjustment);

	void updatePlacement(const QPoint& anchorpoint, const QSize& size);

signals:
	void windowChanged();
	QSDOC_HIDE void backingWindowVisibilityChanged();
	void rectChanged();
	void edgesChanged();
	void gravityChanged();
	void adjustmentChanged();

private slots:
	void onWindowDestroyed();

private:
	QObject* mWindow = nullptr;
	ProxyWindowBase* mProxyWindow = nullptr;
	PopupAnchorState state;
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
