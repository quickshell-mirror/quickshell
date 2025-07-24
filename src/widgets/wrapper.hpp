#pragma once

#include <qflags.h>
#include <qobject.h>
#include <qpointer.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qquickitem.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"

namespace qs::widgets {

///! Helper object for creating components with a single visual child.
/// WrapperManager determines which child of an Item should be its visual
/// child, and exposes it for further operations. See @@MarginWrapperManager
/// for a subclass that implements automatic sizing and margins.
///
/// ### Using wrapper types
/// WrapperManager based types have a single visual child item.
/// You can specify the child item using the default property, or by
/// setting the @@child property. You must use the @@child property if
/// the widget has more than one @@QtQuick.Item based child.
///
/// #### Example using the default property
/// ```qml
/// WrapperWidget { // a widget that uses WrapperManager
///   // Putting the item inline uses the default property of WrapperWidget.
///   @@QtQuick.Text { text: "Hello" }
///
///   // Scope does not extend Item, so it can be placed in the
///   // default property without issue.
///   @@Quickshell.Scope {}
/// }
/// ```
///
/// #### Example using the child property
/// ```qml
/// WrapperWidget {
///   @@QtQuick.Text {
///     id: text
///     text: "Hello"
///   }
///
///   @@QtQuick.Text {
///     id: otherText
///     text: "Other Text"
///   }
///
///   // Both text and otherText extend Item, so one must be specified.
///   child: text
/// }
/// ```
///
/// See @@child for more details on how the child property can be used.
///
/// ### Implementing wrapper types
/// In addition to the bundled wrapper types, you can make your own using
/// WrapperManager. To implement a wrapper, create a WrapperManager inside
/// your wrapper component 's default property, then alias a new property
/// to the WrapperManager's @@child property.
///
/// #### Example
/// ```qml
/// Item { // your wrapper component
///   WrapperManager { id: wrapperManager }
///
///   // Allows consumers of your wrapper component to use the child property.
///   property alias child: wrapperManager.child
///
///   // The rest of your component logic. You can use
///   // `wrapperManager.child` or `this.child` to refer to the selected child.
/// }
/// ```
///
/// ### See also
/// - @@WrapperItem - A @@MarginWrapperManager based component that sizes itself
///   to its child.
/// - @@WrapperRectangle - A @@MarginWrapperManager based component that sizes
///   itself to its child, and provides an option to use its border as an inset.
class WrapperManager
    : public QObject
    , public QQmlParserStatus {
	Q_OBJECT;
	QML_ELEMENT;
	Q_INTERFACES(QQmlParserStatus);

	// clang-format off
	/// The wrapper component's selected child.
	///
	/// Setting this property override's WrapperManager's default selection,
	/// and resolve ambiguity when more than one visual child is present.
	/// The property can additionally be defined inline or reference a component
	/// that is not already a child of the wrapper, in which case it will be
	/// reparented to the wrapper. Setting child to `null` will select no child,
	/// and `undefined` will restore the default child.
	///
	/// When read, `child` will always return the (potentially null) selected child,
	/// and not `undefined`.
	Q_PROPERTY(QQuickItem* child READ child WRITE setProspectiveChild RESET unsetChild NOTIFY childChanged FINAL);
	/// The wrapper managed by this manager. Defaults to the manager's parent.
	/// This property may not be changed after Component.onCompleted.
	Q_PROPERTY(QQuickItem* wrapper READ wrapper WRITE setWrapper NOTIFY wrapperChanged FINAL);
	// clang-format on

public:
	explicit WrapperManager(QObject* parent = nullptr): QObject(parent) {}

	void classBegin() override {}
	void componentComplete() override;

	[[nodiscard]] QQuickItem* child() const;
	void setChild(QQuickItem* child);
	void setProspectiveChild(QQuickItem* child);
	void unsetChild();

	[[nodiscard]] QQuickItem* wrapper() const;
	void setWrapper(QQuickItem* wrapper);

signals:
	void childChanged();
	void wrapperChanged();
	QSDOC_HIDE void initializedChildChanged();

private slots:
	void onChildDestroyed();

protected:
	enum Flag : quint8 {
		NoFlags = 0x0,
		NullChild = 0x1,
		HasMultipleChildren = 0x2,
	};
	Q_DECLARE_FLAGS(Flags, Flag);

	void printChildCountWarning() const;
	void updateGeometry();

	virtual void disconnectChild() {}
	virtual void connectChild() {}

	QQuickItem* mWrapper = nullptr;
	QQuickItem* mAssignedWrapper = nullptr;
	QPointer<QQuickItem> mDefaultChild;
	QQuickItem* mChild = nullptr;
	Flags flags;
};

} // namespace qs::widgets
