#pragma once

#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qicon.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/imageprovider.hpp"
#include "../../core/logcat.hpp"
#include "../../dbus/dbusmenu/dbusmenu.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_item.h"
#include "dbus_item_types.hpp"

QS_DECLARE_LOGGING_CATEGORY(logStatusNotifierItem);

namespace qs::service::sni {

///! Status of a StatusNotifierItem.
/// See @@StatusNotifierItem.status.
namespace Status { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
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

enum Enum : quint8 {
	/// Hardware controls like battery indicators or volume control.
	Hardware = 0,
	/// System services such as IMEs or disk indexing.
	SystemServices = 1,
	/// The fallback category for general applications or anything that does
	/// not fit into a different category.
	ApplicationStatus = 2,
	/// Communication related applications such as instant messengers or email clients.
	Communications = 3,
};
Q_ENUM_NS(Enum);
} // namespace Category

} // namespace qs::service::sni

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::sni::Status::Enum> {
	using Wire = QString;
	using Data = qs::service::sni::Status::Enum;
	static DBusResult<Data> fromWire(const QString& wire);
};

template <>
struct DBusDataTransform<qs::service::sni::Category::Enum> {
	using Wire = QString;
	using Data = qs::service::sni::Category::Enum;
	static DBusResult<Data> fromWire(const QString& wire);
};

} // namespace qs::dbus

namespace qs::service::sni {

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

	// clang-format off
	/// A name unique to the application, such as its name.
	Q_PROPERTY(QString id READ default NOTIFY idChanged BINDABLE bindableId);
	/// Text that describes the application.
	Q_PROPERTY(QString title READ default NOTIFY titleChanged BINDABLE bindableTitle);
	Q_PROPERTY(qs::service::sni::Status::Enum status READ default NOTIFY statusChanged BINDABLE bindableStatus);
	Q_PROPERTY(qs::service::sni::Category::Enum category READ default NOTIFY categoryChanged BINDABLE bindableCategory);
	/// Icon source string, usable as an Image source.
	Q_PROPERTY(QString icon READ default NOTIFY iconChanged BINDABLE bindableIcon);
	Q_PROPERTY(QString tooltipTitle READ tooltipTitle NOTIFY tooltipTitleChanged);
	Q_PROPERTY(QString tooltipDescription READ tooltipDescription NOTIFY tooltipDescriptionChanged);
	/// If this tray item has an associated menu accessible via @@display() or @@menu.
	Q_PROPERTY(bool hasMenu READ default NOTIFY hasMenuChanged BINDABLE bindableHasMenu);
	/// A handle to the menu associated with this tray item, if any.
	///
	/// Can be displayed with @@Quickshell.QsMenuAnchor or @@Quickshell.QsMenuOpener.
	Q_PROPERTY(qs::dbus::dbusmenu::DBusMenuHandle* menu READ menuHandle NOTIFY hasMenuChanged);
	/// If this tray item only offers a menu and activation will do nothing.
	Q_PROPERTY(bool onlyMenu READ default NOTIFY onlyMenuChanged BINDABLE bindableOnlyMenu);
	// clang-format on
	QML_NAMED_ELEMENT(SystemTrayItem);
	QML_UNCREATABLE("SystemTrayItems can only be acquired from SystemTray");

public:
	explicit StatusNotifierItem(const QString& address, QObject* parent = nullptr);

	/// Primary activation action, generally triggered via a left click.
	Q_INVOKABLE void activate();
	/// Secondary activation action, generally triggered via a middle click.
	Q_INVOKABLE void secondaryActivate();
	/// Scroll action, such as changing volume on a mixer.
	Q_INVOKABLE void scroll(qint32 delta, bool horizontal) const;
	/// Display a platform menu at the given location relative to the parent window.
	Q_INVOKABLE void display(QObject* parentWindow, qint32 relativeX, qint32 relativeY);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] bool isReady() const;
	[[nodiscard]] QBindable<QString> bindableIcon() const { return &this->bIcon; }
	[[nodiscard]] QPixmap createPixmap(const QSize& size) const;

	[[nodiscard]] dbus::dbusmenu::DBusMenuHandle* menuHandle();

	[[nodiscard]] QBindable<QString> bindableId() const { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableTitle() const { return &this->bTitle; }
	[[nodiscard]] QBindable<Status::Enum> bindableStatus() const { return &this->bStatus; }
	[[nodiscard]] QBindable<Category::Enum> bindableCategory() const { return &this->bCategory; }
	[[nodiscard]] QString tooltipTitle() const { return this->bTooltip.value().title; }
	[[nodiscard]] QString tooltipDescription() const { return this->bTooltip.value().description; }
	[[nodiscard]] QBindable<bool> bindableHasMenu() const { return &this->bHasMenu; }
	[[nodiscard]] QBindable<bool> bindableOnlyMenu() const { return &this->bIsMenu; }

signals:
	void ready();

	void idChanged();
	void titleChanged();
	void iconChanged();
	void statusChanged();
	void categoryChanged();
	void tooltipTitleChanged();
	void tooltipDescriptionChanged();
	void hasMenuChanged();
	void onlyMenuChanged();

private slots:
	void onGetAllFinished();
	void onGetAllFailed() const;

private:
	void updateMenuState();
	void updatePixmapIndex();
	void onMenuPathChanged();

	DBusStatusNotifierItem* item = nullptr;
	TrayImageHandle imageHandle {this};
	bool mReady = false;

	dbus::dbusmenu::DBusMenuHandle mMenuHandle {this};

	QString watcherId;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bId, &StatusNotifierItem::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bTitle, &StatusNotifierItem::titleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, Status::Enum, bStatus, &StatusNotifierItem::statusChanged);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, Category::Enum, bCategory, &StatusNotifierItem::categoryChanged);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bIconThemePath);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bIconName);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bOverlayIconName);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bAttentionIconName);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, DBusSniIconPixmapList, bIconPixmaps);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, DBusSniIconPixmapList, bOverlayIconPixmaps);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, DBusSniIconPixmapList, bAttentionIconPixmaps);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bAttentionMovieName);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, DBusSniTooltip, bTooltip);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, bool, bIsMenu);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QDBusObjectPath, bMenuPath);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, bool, bHasMenu, &StatusNotifierItem::hasMenuChanged);

	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bIconThemePath, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bIconName, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bOverlayIconName, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bAttentionIconName, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bIconPixmaps, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bOverlayIconPixmaps, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bAttentionIconPixmaps, updatePixmapIndex, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(StatusNotifierItem, bMenuPath, onMenuPathChanged, onValueChanged);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, quint32, pixmapIndex);
	Q_OBJECT_BINDABLE_PROPERTY(StatusNotifierItem, QString, bIcon, &StatusNotifierItem::iconChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(StatusNotifierItem, properties);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pId, bId, properties, "Id");
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pTitle, bTitle, properties, "Title");
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pStatus, bStatus, properties, "Status");
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pCategory, bCategory, properties, "Category", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pIconThemePath, bIconThemePath, properties, "IconThemePath", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pIconName, bIconName, properties, "IconName", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pIconPixmaps, bIconPixmaps, properties, "IconPixmap", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pOverlayIconName, bOverlayIconName, properties, "OverlayIconName", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pOverlayIconPixmaps, bOverlayIconPixmaps, properties, "OverlayIconPixmap", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pAttentionIconName, bAttentionIconName, properties, "AttentionIconName", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pAttentionIconPixmaps, bAttentionIconPixmaps, properties, "AttentionIconPixmap", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pAttentionMovieName, bAttentionMovieName, properties, "AttentionMovieName", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pTooltip, bTooltip, properties, "ToolTip", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pIsMenu, bIsMenu, properties, "ItemIsMenu", false);
	QS_DBUS_PROPERTY_BINDING(StatusNotifierItem, pMenuPath, bMenuPath, properties, "Menu", false);
	// clang-format on
};

} // namespace qs::service::sni
