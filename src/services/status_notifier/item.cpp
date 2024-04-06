#include "item.hpp"
#include <utility>

#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qicon.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../../dbus/dbusutil.hpp"
#include "dbus_item.h"
#include "dbus_item_types.hpp"
#include "host.hpp"

using namespace qs::dbus;

Q_LOGGING_CATEGORY(logStatusNotifierItem, "quickshell.service.sni.item", QtWarningMsg);

namespace qs::service::sni {

StatusNotifierItem::StatusNotifierItem(const QString& address, QObject* parent): QObject(parent) {
	// spec is unclear about what exactly an item address is, so split off anything but the connection path
	auto conn = address.split("/").value(0);
	this->item =
	    new DBusStatusNotifierItem(conn, "/StatusNotifierItem", QDBusConnection::sessionBus(), this);

	if (!this->item->isValid()) {
		qCWarning(logStatusNotifierHost).noquote() << "Cannot create StatusNotifierItem for" << conn;
		return;
	}

	qDBusRegisterMetaType<DBusSniIconPixmap>();
	qDBusRegisterMetaType<DBusSniIconPixmapList>();
	qDBusRegisterMetaType<DBusSniTooltip>();

	// clang-format off
	QObject::connect(this->item, &DBusStatusNotifierItem::NewTitle, &this->title, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->iconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->iconPixmaps, &AbstractDBusProperty::update);
	//QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->iconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->overlayIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->overlayIconPixmaps, &AbstractDBusProperty::update);
	//QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->iconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->attentionIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->attentionIconPixmaps, &AbstractDBusProperty::update);
	//QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->iconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewToolTip, &this->tooltip, &AbstractDBusProperty::update);

	QObject::connect(&this->iconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->attentionIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->overlayIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->iconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->attentionIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->overlayIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);

	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFinished, this, &StatusNotifierItem::onGetAllFinished);
	// clang-format on

	QObject::connect(this->item, &DBusStatusNotifierItem::NewStatus, this, [this](QString value) {
		qCDebug(logStatusNotifierItem) << "Received update for" << this->status.toString() << value;
		this->status.set(std::move(value));
	});

	this->properties.setInterface(this->item);
	this->properties.updateAllViaGetAll();
}

bool StatusNotifierItem::isValid() const { return this->item->isValid(); }
bool StatusNotifierItem::isReady() const { return this->mReady; }

QString StatusNotifierItem::iconId() const {
	if (this->status.get() == "NeedsAttention") {
		auto name = this->attentionIconName.get();
		if (!name.isEmpty()) return QString("image://icon/") + name;
	} else {
		auto name = this->iconName.get();
		auto overlayName = this->overlayIconName.get();
		if (!name.isEmpty() && overlayName.isEmpty()) return QString("image://icon/") + name;
	}

	return QString("image://service.sni/") + this->item->service() + "/"
	     + QString::number(this->iconIndex);
}

QPixmap StatusNotifierItem::createPixmap(const QSize& size) const {
	auto needsAttention = this->status.get() == "NeedsAttention";

	auto closestPixmap = [](const QSize& size, const DBusSniIconPixmapList& pixmaps) {
		const DBusSniIconPixmap* ret = nullptr;

		for (const auto& pixmap: pixmaps) {
			if (ret == nullptr) {
				ret = &pixmap;
				continue;
			}

			auto existingAdequate = ret->width >= size.width() && ret->height >= size.height();
			auto newAdequite = pixmap.width >= size.width() && pixmap.height >= size.height();
			auto newSmaller = pixmap.width < ret->width || pixmap.height < ret->height;

			if ((existingAdequate && newAdequite && newSmaller) || (!existingAdequate && !newSmaller)) {
				ret = &pixmap;
			}
		}

		return ret;
	};

	QPixmap pixmap;
	if (needsAttention) {
		if (!this->attentionIconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->attentionIconName.get());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->attentionIconPixmaps.get());
			if (icon != nullptr) pixmap = QPixmap::fromImage(icon->createImage());
		}
	} else {
		if (!this->iconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->iconName.get());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->iconPixmaps.get());
			if (icon != nullptr) pixmap = QPixmap::fromImage(icon->createImage());
		}

		QPixmap overlay;
		if (!this->overlayIconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->overlayIconName.get());
			overlay = icon.pixmap(pixmap.width(), pixmap.height());
		} else {
			const auto* icon = closestPixmap(pixmap.size(), this->overlayIconPixmaps.get());
			if (icon != nullptr) overlay = QPixmap::fromImage(icon->createImage());
		}

		if (!overlay.isNull()) {
			auto painter = QPainter(&pixmap);
			painter.drawPixmap(QRect(0, 0, pixmap.width(), pixmap.height()), overlay);
			painter.end();
		}
	}

	return pixmap;
}

void StatusNotifierItem::updateIcon() {
	this->iconIndex++;
	emit this->iconChanged();
}

void StatusNotifierItem::onGetAllFinished() {
	if (this->mReady) return;
	this->mReady = true;
	emit this->ready();
}

} // namespace qs::service::sni
