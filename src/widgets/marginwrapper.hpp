#pragma once

#include <qflags.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "wrapper.hpp"

namespace qs::widgets {

///! Helper object for applying sizes and margins to a single child item.
/// > [!NOTE] MarginWrapperManager is an extension of @@WrapperManager.
/// > You should read its documentation to understand wrapper types.
///
/// MarginWrapperManager can be used to apply margins to a child item,
/// in addition to handling the size / implicit size relationship
/// between the parent and the child. @@WrapperItem and @@WrapperRectangle
/// exist for Item and Rectangle implementations respectively.
///
/// > [!WARNING] MarginWrapperManager based types set the child item's
/// > @@QtQuick.Item.x, @@QtQuick.Item.y, @@QtQuick.Item.width, @@QtQuick.Item.height
/// > or @@QtQuick.Item.anchors properties. Do not set them yourself,
/// > instead set @@Item.implicitWidth and @@Item.implicitHeight.
///
/// ### Implementing a margin wrapper type
/// Follow the directions in @@WrapperManager$'s documentation, and or
/// alias the @@margin property if you wish to expose it.
///
/// ## Margin calculation
/// The margin of the content item is calculated based on @@topMargin, @@bottomMargin,
/// @@leftMargin, @@rightMargin, @@extraMargin and @@resizeChild.
///
/// If @@resizeChild is `true`, each side's margin will be the value of `<side>Margin`
/// plus @@extraMargin, and the content item will be stretched to match the given margin
/// if the wrapper is not at its implicit size.
///
/// If @@resizeChild is `false`, the `<side>Margin` properties will be interpreted as a
/// ratio and the content item will not be stretched if the wrapper is not at its implicit side.
///
/// The implicit size of the wrapper is the implicit size of the content item
/// plus all margins.
class MarginWrapperManager: public WrapperManager {
	Q_OBJECT;
	// clang-format off
	/// The default for @@topMargin, @@bottomMargin, @@leftMargin and @@rightMargin.
	/// Defaults to 0.
	Q_PROPERTY(qreal margin READ default WRITE default BINDABLE bindableMargin NOTIFY marginChanged FINAL);
	/// An extra margin applied in addition to @@topMargin, @@bottomMargin,
  /// @@leftMargin, and @@rightMargin. Defaults to 0.
	Q_PROPERTY(qreal extraMargin READ default WRITE default BINDABLE bindableExtraMargin NOTIFY baseMarginChanged FINAL);
	/// The requested top margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin RESET resetTopMargin NOTIFY topMarginChanged FINAL);
	/// The requested bottom margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin RESET resetBottomMargin NOTIFY bottomMarginChanged FINAL);
	/// The requested left margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin RESET resetLeftMargin NOTIFY leftMarginChanged FINAL);
	/// The requested right margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin RESET resetRightMargin NOTIFY rightMarginChanged FINAL);
	/// Determines if child item should be resized larger than its implicit size if
	/// the parent is resized larger than its implicit size. Defaults to true.
	Q_PROPERTY(bool resizeChild READ default WRITE default BINDABLE bindableResizeChild NOTIFY resizeChildChanged FINAL);
	/// Overrides the implicit width of the wrapper.
	///
	/// Defaults to the implicit width of the content item plus its left and right margin,
	/// and may be reset by assigning `undefined`.
	Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth RESET resetImplicitWidth NOTIFY implicitWidthChanged FINAL);
	/// Overrides the implicit height of the wrapper.
	///
	/// Defaults to the implicit width of the content item plus its top and bottom margin,
	/// and may be reset by assigning `undefined`.
	Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight RESET resetImplicitHeight NOTIFY implicitHeightChanged FINAL);
	// clang-format on
	QML_ELEMENT;

public:
	explicit MarginWrapperManager(QObject* parent = nullptr);

	void componentComplete() override;

	[[nodiscard]] QBindable<qreal> bindableMargin() { return &this->bMargin; }
	[[nodiscard]] QBindable<qreal> bindableExtraMargin() { return &this->bExtraMargin; }

	[[nodiscard]] qreal topMargin() const { return this->bTopMargin.value(); }
	void resetTopMargin() { this->bOverrides = this->bOverrides.value() & ~TopMargin; }
	void setTopMargin(qreal topMargin) {
		this->bTopMarginOverride = topMargin;
		this->bOverrides = this->bOverrides.value() | TopMargin;
	}

	[[nodiscard]] qreal bottomMargin() const { return this->bBottomMargin.value(); }
	void resetBottomMargin() { this->bOverrides = this->bOverrides.value() & ~BottomMargin; }
	void setBottomMargin(qreal bottomMargin) {
		this->bBottomMarginOverride = bottomMargin;
		this->bOverrides = this->bOverrides.value() | BottomMargin;
	}

	[[nodiscard]] qreal leftMargin() const { return this->bLeftMargin.value(); }
	void resetLeftMargin() { this->bOverrides = this->bOverrides.value() & ~LeftMargin; }
	void setLeftMargin(qreal leftMargin) {
		this->bLeftMarginOverride = leftMargin;
		this->bOverrides = this->bOverrides.value() | LeftMargin;
	}

	[[nodiscard]] qreal rightMargin() const { return this->bRightMargin.value(); }
	void resetRightMargin() { this->bOverrides = this->bOverrides.value() & ~RightMargin; }
	void setRightMargin(qreal rightMargin) {
		this->bRightMarginOverride = rightMargin;
		this->bOverrides = this->bOverrides.value() | RightMargin;
	}

	[[nodiscard]] QBindable<bool> bindableResizeChild() { return &this->bResizeChild; }

	[[nodiscard]] qreal implicitWidth() const { return this->bWrapperImplicitWidth.value(); }
	void resetImplicitWidth() { this->bOverrides = this->bOverrides.value() & ~ImplicitWidth; }
	void setImplicitWidth(qreal implicitWidth) {
		this->bImplicitWidthOverride = implicitWidth;
		this->bOverrides = this->bOverrides.value() | ImplicitWidth;
	}

	[[nodiscard]] qreal implicitHeight() const { return this->bWrapperImplicitHeight.value(); }
	void resetImplicitHeight() { this->bOverrides = this->bOverrides.value() & ~ImplicitHeight; }
	void setImplicitHeight(qreal implicitHeight) {
		this->bImplicitHeightOverride = implicitHeight;
		this->bOverrides = this->bOverrides.value() | ImplicitHeight;
	}

	// has to be public for flag operator definitions
	enum OverrideFlag : quint8 {
		ImplicitWidth = 0b1,
		ImplicitHeight = 0b10,
		TopMargin = 0b100,
		BottomMargin = 0b1000,
		LeftMargin = 0b10000,
		RightMargin = 0b100000,
	};

	Q_DECLARE_FLAGS(OverrideFlags, OverrideFlag);

signals:
	void marginChanged();
	void baseMarginChanged();
	void topMarginChanged();
	void bottomMarginChanged();
	void leftMarginChanged();
	void rightMarginChanged();
	void resizeChildChanged();
	void implicitWidthChanged();
	void implicitHeightChanged();

private slots:
	void onChildImplicitWidthChanged();
	void onChildImplicitHeightChanged();
	void setWrapperImplicitWidth();
	void setWrapperImplicitHeight();

protected:
	void disconnectChild() override;
	void connectChild() override;

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MarginWrapperManager, bool, bResizeChild, true);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bMargin, &MarginWrapperManager::marginChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bExtraMargin, &MarginWrapperManager::baseMarginChanged);

	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, OverrideFlags, bOverrides);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bImplicitWidthOverride);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bImplicitHeightOverride);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bTopMarginOverride);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bBottomMarginOverride);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bLeftMarginOverride);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bRightMarginOverride);

	// computed
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bTopMargin, &MarginWrapperManager::topMarginChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bBottomMargin, &MarginWrapperManager::bottomMarginChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bLeftMargin, &MarginWrapperManager::leftMarginChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bRightMargin, &MarginWrapperManager::rightMarginChanged);

	// bound
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bWrapperWidth);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bWrapperHeight);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bChildImplicitWidth);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bChildImplicitHeight);

	// computed
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bChildX);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bChildY);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bChildWidth);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bChildHeight);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bWrapperImplicitWidth, &MarginWrapperManager::setWrapperImplicitWidth);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bWrapperImplicitHeight, &MarginWrapperManager::setWrapperImplicitHeight);

	// clang-format on
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MarginWrapperManager::OverrideFlags);

} // namespace qs::widgets
