#pragma once

#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qicon.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/imageprovider.hpp"
#include "../../dbus/dbusmenu/dbusmenu.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_item.h"
#include "dbus_item_types.hpp"

Q_DECLARE_LOGGING_CATEGORY(logStatusNotifierItem);

namespace qs::service::sni {

///! Status of a StatusNotifierItem.
/// See @@StatusNotifierItem.status.
namespace Status { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	/// A passive item does not convey important information and can be considered idle. You may want to hide these.
	Passive = 0,
	/// An active item may have information more important than a passive one and you probably do not want to hide it.
	Active = 1,
	/// An item that needs attention conveys very important information such as low battery.
	NeedsAttention = 2,
};
Q_ENUM_NS(Enum);
} // namespace Status

///! Category of a StatusNotifierItem.
/// See @@StatusNotifierItem.category.
namespace Category { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	/// The fallback category for general applications or anything that does
	/// not fit into a different category.
	ApplicationStatus = 0,
	/// System services such as IMEs or disk indexing.
	SystemServices = 1,
	/// Hardware controls like battery indicators or volume control.
	Hardware = 2,
};
Q_ENUM_NS(Enum);
} // namespace Category

class StatusNotifierItem;

class TrayImageHandle: public QsImageHandle {
public:
	explicit TrayImageHandle(StatusNotifierItem* item);

	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

public:
	StatusNotifierItem* item;
};

///! An item in the system tray.
/// A system tray item, roughly conforming to the [kde/freedesktop spec]
/// (there is no real spec, we just implemented whatever seemed to actually be used).
///
/// [kde/freedesktop spec]: https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem/
class StatusNotifierItem: public QObject {
	Q_OBJECT;

	/// A name unique to the application, such as its name.
	Q_PROPERTY(QString id READ id NOTIFY idChanged);
	/// Text that describes the application.
	Q_PROPERTY(QString title READ title NOTIFY titleChanged);
	Q_PROPERTY(qs::service::sni::Status::Enum status READ status NOTIFY statusChanged);
	Q_PROPERTY(qs::service::sni::Category::Enum category READ category NOTIFY categoryChanged);
	/// Icon source string, usable as an Image source.
	Q_PROPERTY(QString icon READ iconId NOTIFY iconChanged);
	Q_PROPERTY(QString tooltipTitle READ tooltipTitle NOTIFY tooltipTitleChanged);
	Q_PROPERTY(QString tooltipDescription READ tooltipDescription NOTIFY tooltipDescriptionChanged);
	/// If this tray item has an associated menu accessible via @@display() or @@menu.
	Q_PROPERTY(bool hasMenu READ hasMenu NOTIFY hasMenuChanged);
	/// A handle to the menu associated with this tray item, if any.
	///
	/// Can be displayed with @@Quickshell.QsMenuAnchor or @@Quickshell.QsMenuOpener.
	Q_PROPERTY(qs::dbus::dbusmenu::DBusMenuHandle* menu READ menuHandle NOTIFY hasMenuChanged);
	/// If this tray item only offers a menu and activation will do nothing.
	Q_PROPERTY(bool onlyMenu READ onlyMenu NOTIFY onlyMenuChanged);
	QML_NAMED_ELEMENT(SystemTrayItem);
	QML_UNCREATABLE("SystemTrayItems can only be acquired from SystemTray");

public:
	explicit StatusNotifierItem(const QString& address, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] bool isReady() const;
	[[nodiscard]] QString iconId() const;
	[[nodiscard]] QPixmap createPixmap(const QSize& size) const;

	[[nodiscard]] dbus::dbusmenu::DBusMenuHandle* menuHandle();

	/// Primary activation action, generally triggered via a left click.
	void activate();
	/// Secondary activation action, generally triggered via a middle click.
	void secondaryActivate();
	/// Scroll action, such as changing volume on a mixer.
	void scroll(qint32 delta, bool horizontal) const;
	/// Display a platform menu at the given location relative to the parent window.
	void display(QObject* parentWindow, qint32 relativeX, qint32 relativeY);

	[[nodiscard]] QString id() const;
	[[nodiscard]] QString title() const;
	[[nodiscard]] Status::Enum status() const;
	[[nodiscard]] Category::Enum category() const;
	[[nodiscard]] QString tooltipTitle() const;
	[[nodiscard]] QString tooltipDescription() const;
	[[nodiscard]] bool hasMenu() const;
	[[nodiscard]] bool onlyMenu() const;

signals:
	void iconChanged();
	void ready();

	void idChanged();
	void titleChanged();
	void statusChanged();
	void categoryChanged();
	void tooltipTitleChanged();
	void tooltipDescriptionChanged();
	void hasMenuChanged();
	void onlyMenuChanged();

private slots:
	void updateIcon();
	void onGetAllFinished();
	void onGetAllFailed() const;
	void onMenuPathChanged();

private:
	void updateMenuState();

	DBusStatusNotifierItem* item = nullptr;
	TrayImageHandle imageHandle {this};
	bool mReady = false;

	dbus::dbusmenu::DBusMenuHandle mMenuHandle {this};

	// bumped to inhibit caching
	quint32 iconIndex = 0;
	QString watcherId;

	// clang-format off
	dbus::DBusPropertyGroup properties;
	dbus::DBusProperty<QString> pId {this->properties, "Id"};
	dbus::DBusProperty<QString> pTitle {this->properties, "Title"};
	dbus::DBusProperty<QString> pStatus {this->properties, "Status"};
	dbus::DBusProperty<QString> pCategory {this->properties, "Category"};
	dbus::DBusProperty<quint32> pWindowId {this->properties, "WindowId"};
	dbus::DBusProperty<QString> pIconThemePath {this->properties, "IconThemePath", "", false};
	dbus::DBusProperty<QString> pIconName {this->properties, "IconName", "", false}; // IconPixmap may be set
	dbus::DBusProperty<DBusSniIconPixmapList> pIconPixmaps {this->properties, "IconPixmap", {}, false}; // IconName may be set
	dbus::DBusProperty<QString> pOverlayIconName {this->properties, "OverlayIconName"};
	dbus::DBusProperty<DBusSniIconPixmapList> pOverlayIconPixmaps {this->properties, "OverlayIconPixmap"};
	dbus::DBusProperty<QString> pAttentionIconName {this->properties, "AttentionIconName"};
	dbus::DBusProperty<DBusSniIconPixmapList> pAttentionIconPixmaps {this->properties, "AttentionIconPixmap"};
	dbus::DBusProperty<QString> pAttentionMovieName {this->properties, "AttentionMovieName", "", false};
	dbus::DBusProperty<DBusSniTooltip> pTooltip {this->properties, "ToolTip"};
	dbus::DBusProperty<bool> pIsMenu {this->properties, "ItemIsMenu"};
	dbus::DBusProperty<QDBusObjectPath> pMenuPath {this->properties, "Menu"};
	// clang-format on
};

} // namespace qs::service::sni
