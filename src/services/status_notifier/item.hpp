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

class StatusNotifierItem;

class TrayImageHandle: public QsImageHandle {
public:
	explicit TrayImageHandle(StatusNotifierItem* item);

	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

public:
	StatusNotifierItem* item;
};

class StatusNotifierItem: public QObject {
	Q_OBJECT;

public:
	explicit StatusNotifierItem(const QString& address, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] bool isReady() const;
	[[nodiscard]] QString iconId() const;
	[[nodiscard]] QPixmap createPixmap(const QSize& size) const;

	[[nodiscard]] dbus::dbusmenu::DBusMenuHandle* menuHandle();

	void activate();
	void secondaryActivate();
	void scroll(qint32 delta, bool horizontal);

	// clang-format off
	dbus::DBusPropertyGroup properties;
	dbus::DBusProperty<QString> id {this->properties, "Id"};
	dbus::DBusProperty<QString> title {this->properties, "Title"};
	dbus::DBusProperty<QString> status {this->properties, "Status"};
	dbus::DBusProperty<QString> category {this->properties, "Category"};
	dbus::DBusProperty<quint32> windowId {this->properties, "WindowId"};
	dbus::DBusProperty<QString> iconThemePath {this->properties, "IconThemePath", "", false};
	dbus::DBusProperty<QString> iconName {this->properties, "IconName", "", false}; // IconPixmap may be set
	dbus::DBusProperty<DBusSniIconPixmapList> iconPixmaps {this->properties, "IconPixmap", {}, false}; // IconName may be set
	dbus::DBusProperty<QString> overlayIconName {this->properties, "OverlayIconName"};
	dbus::DBusProperty<DBusSniIconPixmapList> overlayIconPixmaps {this->properties, "OverlayIconPixmap"};
	dbus::DBusProperty<QString> attentionIconName {this->properties, "AttentionIconName"};
	dbus::DBusProperty<DBusSniIconPixmapList> attentionIconPixmaps {this->properties, "AttentionIconPixmap"};
	dbus::DBusProperty<QString> attentionMovieName {this->properties, "AttentionMovieName", "", false};
	dbus::DBusProperty<DBusSniTooltip> tooltip {this->properties, "ToolTip"};
	dbus::DBusProperty<bool> isMenu {this->properties, "ItemIsMenu"};
	dbus::DBusProperty<QDBusObjectPath> menuPath {this->properties, "Menu"};
	// clang-format on

signals:
	void iconChanged();
	void ready();

private slots:
	void updateIcon();
	void onGetAllFinished();
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
};

} // namespace qs::service::sni
