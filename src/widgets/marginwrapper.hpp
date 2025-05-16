#pragma once

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
	/// the parent is resized larger than its implicit size. Defaults to false.
	Q_PROPERTY(bool resizeChild READ default WRITE default BINDABLE bindableResizeChild NOTIFY resizeChildChanged FINAL);
	// clang-format on
	QML_ELEMENT;

public:
	explicit MarginWrapperManager(QObject* parent = nullptr);

	void componentComplete() override;

	[[nodiscard]] QBindable<qreal> bindableMargin() { return &this->bMargin; }
	[[nodiscard]] QBindable<qreal> bindableExtraMargin() { return &this->bExtraMargin; }

	[[nodiscard]] qreal topMargin() const { return this->bTopMargin.value(); }
	void resetTopMargin() { this->bTopMarginSet = false; }
	void setTopMargin(qreal topMargin) {
		this->bTopMarginValue = topMargin;
		this->bTopMarginSet = true;
	}

	[[nodiscard]] qreal bottomMargin() const { return this->bBottomMargin.value(); }
	void resetBottomMargin() { this->bBottomMarginSet = false; }
	void setBottomMargin(qreal bottomMargin) {
		this->bBottomMarginValue = bottomMargin;
		this->bBottomMarginSet = true;
	}

	[[nodiscard]] qreal leftMargin() const { return this->bLeftMargin.value(); }
	void resetLeftMargin() { this->bLeftMarginSet = false; }
	void setLeftMargin(qreal leftMargin) {
		this->bLeftMarginValue = leftMargin;
		this->bLeftMarginSet = true;
	}

	[[nodiscard]] qreal rightMargin() const { return this->bRightMargin.value(); }
	void resetRightMargin() { this->bRightMarginSet = false; }
	void setRightMargin(qreal rightMargin) {
		this->bRightMarginValue = rightMargin;
		this->bRightMarginSet = true;
	}

	[[nodiscard]] QBindable<bool> bindableResizeChild() { return &this->bResizeChild; }

signals:
	void marginChanged();
	void baseMarginChanged();
	void topMarginChanged();
	void bottomMarginChanged();
	void leftMarginChanged();
	void rightMarginChanged();
	void resizeChildChanged();

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
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, bool, bResizeChild);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bMargin, &MarginWrapperManager::marginChanged);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bExtraMargin, &MarginWrapperManager::baseMarginChanged);

	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, bool, bTopMarginSet);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, bool, bBottomMarginSet);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, bool, bLeftMarginSet);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, bool, bRightMarginSet);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bTopMarginValue);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bBottomMarginValue);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bLeftMarginValue);
	Q_OBJECT_BINDABLE_PROPERTY(MarginWrapperManager, qreal, bRightMarginValue);

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

} // namespace qs::widgets
