#pragma once

#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickimageprovider.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/imageprovider.hpp"
#include "../properties.hpp"
#include "dbus_menu_types.hpp"

Q_DECLARE_LOGGING_CATEGORY(logDbusMenu);

namespace ToggleButtonType { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	/// This menu item does not have a checkbox or a radiobutton associated with it.
	None = 0,
	/// This menu item should draw a checkbox.
	CheckBox = 1,
	/// This menu item should draw a radiobutton.
	RadioButton = 2,
};
Q_ENUM_NS(Enum);

} // namespace ToggleButtonType

class DBusMenuInterface;

namespace qs::dbus::dbusmenu {

QDebug operator<<(QDebug debug, const ToggleButtonType::Enum& toggleType);

class DBusMenu;
class DBusMenuPngImage;

///! Menu item shared by an external program.
/// Menu item shared by an external program via the
/// [DBusMenu specification](https://github.com/AyatanaIndicators/libdbusmenu/blob/master/libdbusmenu-glib/dbus-menu.xml).
class DBusMenuItem: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Handle to the root of this menu.
	Q_PROPERTY(DBusMenu* menuHandle READ menuHandle CONSTANT);
	/// Text of the menu item, including hotkey markup.
	Q_PROPERTY(QString label READ label NOTIFY labelChanged);
	/// Text of the menu item without hotkey markup.
	Q_PROPERTY(QString cleanLabel READ cleanLabel NOTIFY labelChanged);
  /// If the menu item should be shown as enabled.
	///
	/// > [!INFO] Disabled menu items are often used as headers in addition
	/// > to actual disabled entries.
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged);
	/// Url of the menu item's icon or `""` if it doesn't have one.
	///
	/// This can be passed to [Image.source](https://doc.qt.io/qt-6/qml-qtquick-image.html#source-prop)
	/// as shown below.
	///
	/// ```qml
	/// Image {
	///   source: menuItem.icon
	///   // To get the best image quality, set the image source size to the same size
	///   // as the rendered image.
	///   sourceSize.width: width
	///   sourceSize.height: height
	/// }
	/// ```
	Q_PROPERTY(QString icon READ icon NOTIFY iconChanged);
	/// If this menu item has an associated checkbox or radiobutton.
	///
	/// > [!INFO] It is the responsibility of the remote application to update the state of
	/// > checkboxes and radiobuttons via [checkState](#prop.checkState).
	Q_PROPERTY(ToggleButtonType::Enum toggleType READ toggleType NOTIFY toggleTypeChanged);
	/// The check state of the checkbox or radiobutton if applicable, as a
  /// [Qt.CheckState](https://doc.qt.io/qt-6/qt.html#CheckState-enum).
	Q_PROPERTY(Qt::CheckState checkState READ checkState NOTIFY checkStateChanged);
	/// If this menu item should be rendered as a separator between other items.
	///
	/// No other properties have a meaningful value when `isSeparator` is true.
	Q_PROPERTY(bool isSeparator READ isSeparator NOTIFY separatorChanged);
	/// If this menu item reveals a submenu containing more items.
	///
	/// Any submenu items must be requested by setting [showChildren](#prop.showChildren).
	Q_PROPERTY(bool hasChildren READ hasChildren NOTIFY hasChildrenChanged);
	/// If submenu entries of this item should be shown.
	///
	/// When true, children of this menu item will be exposed via [children](#prop.children).
	/// Setting this property will additionally send the `opened` and `closed` events to the
	/// process that provided the menu.
	Q_PROPERTY(bool showChildren READ isShowingChildren WRITE setShowChildren NOTIFY showingChildrenChanged);
	/// Children of this menu item. Only populated when [showChildren](#prop.showChildren) is true.
	///
	/// > [!INFO] Use [hasChildren](#prop.hasChildren) to check if this item should reveal a submenu
	/// > instead of checking if `children` is empty.
	Q_PROPERTY(QQmlListProperty<DBusMenuItem> children READ children NOTIFY childrenChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("DBusMenus can only be acquired from a DBusMenuHandle");

public:
	explicit DBusMenuItem(qint32 id, DBusMenu* menu, DBusMenuItem* parentMenu);

	/// Send a `clicked` event to the remote application for this menu item.
	Q_INVOKABLE void click();

	/// Send a `hovered` event to the remote application for this menu item.
	///
	/// Note: we are not aware of any programs that use this in any meaningful way.
	Q_INVOKABLE void hover() const;

	[[nodiscard]] DBusMenu* menuHandle() const;
	[[nodiscard]] QString label() const;
	[[nodiscard]] QString cleanLabel() const;
	[[nodiscard]] bool enabled() const;
	[[nodiscard]] QString icon() const;
	[[nodiscard]] ToggleButtonType::Enum toggleType() const;
	[[nodiscard]] Qt::CheckState checkState() const;
	[[nodiscard]] bool isSeparator() const;
	[[nodiscard]] bool hasChildren() const;

	[[nodiscard]] bool isShowingChildren() const;
	void setShowChildren(bool showChildren);

	[[nodiscard]] QQmlListProperty<DBusMenuItem> children();

	void updateProperties(const QVariantMap& properties, const QStringList& removed = {});
	void onChildrenUpdated();

	qint32 id = 0;
	QString mLabel;
	QVector<qint32> mChildren;
	bool mShowChildren = false;
	bool childrenLoaded = false;
	DBusMenu* menu = nullptr;

signals:
	void labelChanged();
	//void mnemonicChanged();
	void enabledChanged();
	void iconChanged();
	void separatorChanged();
	void toggleTypeChanged();
	void checkStateChanged();
	void hasChildrenChanged();
	void showingChildrenChanged();
	void childrenChanged();

private:
	QString mCleanLabel;
	//QChar mnemonic;
	bool mEnabled = true;
	bool visible = true;
	bool mSeparator = false;
	QString iconName;
	DBusMenuPngImage* image = nullptr;
	ToggleButtonType::Enum mToggleType = ToggleButtonType::None;
	Qt::CheckState mCheckState = Qt::Checked;
	bool displayChildren = false;
	QVector<qint32> enabledChildren;
	DBusMenuItem* parentMenu = nullptr;

	static qsizetype childrenCount(QQmlListProperty<DBusMenuItem>* property);
	static DBusMenuItem* childAt(QQmlListProperty<DBusMenuItem>* property, qsizetype index);
};

QDebug operator<<(QDebug debug, DBusMenuItem* item);

///! Handle to a DBusMenu tree.
/// Handle to a menu tree provided by a remote process.
class DBusMenu: public QObject {
	Q_OBJECT;
	Q_PROPERTY(DBusMenuItem* menu READ menu CONSTANT);
	QML_NAMED_ELEMENT(DBusMenuHandle);
	QML_UNCREATABLE("Menu handles cannot be directly created");

public:
	explicit DBusMenu(const QString& service, const QString& path, QObject* parent = nullptr);

	dbus::DBusPropertyGroup properties;
	dbus::DBusProperty<quint32> version {this->properties, "Version"};
	dbus::DBusProperty<QString> textDirection {this->properties, "TextDirection", "", false};
	dbus::DBusProperty<QString> status {this->properties, "Status"};
	dbus::DBusProperty<QStringList> iconThemePath {this->properties, "IconThemePath", {}, false};

	void prepareToShow(qint32 item, bool sendOpened);
	void updateLayout(qint32 parent, qint32 depth);
	void removeRecursive(qint32 id);
	void sendEvent(qint32 item, const QString& event);

	DBusMenuItem rootItem {0, this, nullptr};
	QHash<qint32, DBusMenuItem*> items {std::make_pair(0, &this->rootItem)};

	[[nodiscard]] DBusMenuItem* menu();

private slots:
	void onLayoutUpdated(quint32 revision, qint32 parent);
	void onItemPropertiesUpdated(
	    const DBusMenuItemPropertiesList& updatedProps,
	    const DBusMenuItemPropertyNamesList& removedProps
	);

private:
	void updateLayoutRecursive(const DBusMenuLayout& layout, DBusMenuItem* parent, qint32 depth);

	DBusMenuInterface* interface = nullptr;
};

QDebug operator<<(QDebug debug, DBusMenu* menu);

class DBusMenuPngImage: public QsImageHandle {
public:
	explicit DBusMenuPngImage(QByteArray data, DBusMenuItem* parent)
	    : QsImageHandle(QQuickImageProvider::Image, parent)
	    , data(std::move(data)) {}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

	QByteArray data;
};

} // namespace qs::dbus::dbusmenu
