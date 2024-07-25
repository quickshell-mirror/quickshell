#include "item.hpp"
#include <utility>

#include <qdbusextratypes.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qicon.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qqmlengine.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/iconimageprovider.hpp"
#include "../../core/imageprovider.hpp"
#include "../../dbus/dbusmenu/dbusmenu.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_item.h"
#include "dbus_item_types.hpp"
#include "host.hpp"

using namespace qs::dbus;
using namespace qs::dbus::dbusmenu;

Q_LOGGING_CATEGORY(logStatusNotifierItem, "quickshell.service.sni.item", QtWarningMsg);
Q_LOGGING_CATEGORY(logSniMenu, "quickshell.service.sni.item.menu", QtWarningMsg);

namespace qs::service::sni {

StatusNotifierItem::StatusNotifierItem(const QString& address, QObject* parent)
    : QObject(parent)
    , watcherId(address) {
	qDBusRegisterMetaType<DBusSniIconPixmap>();
	qDBusRegisterMetaType<DBusSniIconPixmapList>();
	qDBusRegisterMetaType<DBusSniTooltip>();

	// spec is unclear about what exactly an item address is, so account for both combinations
	auto splitIdx = address.indexOf('/');
	auto conn = splitIdx == -1 ? address : address.sliced(0, splitIdx);
	auto path = splitIdx == -1 ? "/StatusNotifierItem" : address.sliced(splitIdx);

	this->item = new DBusStatusNotifierItem(conn, path, QDBusConnection::sessionBus(), this);

	if (!this->item->isValid()) {
		qCWarning(logStatusNotifierHost).noquote() << "Cannot create StatusNotifierItem for" << conn;
		return;
	}

	// clang-format off
	QObject::connect(this->item, &DBusStatusNotifierItem::NewTitle, &this->title, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->iconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->iconPixmaps, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->iconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->overlayIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->overlayIconPixmaps, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->iconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->attentionIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->attentionIconPixmaps, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->iconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewToolTip, &this->tooltip, &AbstractDBusProperty::update);

	QObject::connect(&this->iconThemePath, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->iconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->attentionIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->overlayIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->iconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->attentionIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->overlayIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);

	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFinished, this, &StatusNotifierItem::onGetAllFinished);
	QObject::connect(&this->menuPath, &AbstractDBusProperty::changed, this, &StatusNotifierItem::onMenuPathChanged);
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
		if (!name.isEmpty()) return IconImageProvider::requestString(name, this->iconThemePath.get());
	} else {
		auto name = this->iconName.get();
		auto overlayName = this->overlayIconName.get();
		if (!name.isEmpty() && overlayName.isEmpty())
			return IconImageProvider::requestString(name, this->iconThemePath.get());
	}

	return this->imageHandle.url() + "/" + QString::number(this->iconIndex);
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

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				pixmap = QPixmap::fromImage(image);
			}
		}
	} else {
		if (!this->iconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->iconName.get());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->iconPixmaps.get());

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				pixmap = QPixmap::fromImage(image);
			}
		}

		QPixmap overlay;
		if (!this->overlayIconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->overlayIconName.get());
			overlay = icon.pixmap(pixmap.width(), pixmap.height());
		} else {
			const auto* icon = closestPixmap(pixmap.size(), this->overlayIconPixmaps.get());

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				overlay = QPixmap::fromImage(image);
			}
		}

		if (!overlay.isNull()) {
			auto painter = QPainter(&pixmap);
			painter.drawPixmap(QRect(0, 0, pixmap.width(), pixmap.height()), overlay);
			painter.end();
		}
	}

	return pixmap;
}

void StatusNotifierItem::activate() {
	auto pendingCall = this->item->Activate(0, 0);
	auto* call = new QDBusPendingCallWatcher(pendingCall, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logStatusNotifierItem).noquote()
			    << "Error calling Activate method of StatusNotifierItem" << this->properties.toString();
			qCWarning(logStatusNotifierItem) << reply.error();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void StatusNotifierItem::secondaryActivate() {
	auto pendingCall = this->item->SecondaryActivate(0, 0);
	auto* call = new QDBusPendingCallWatcher(pendingCall, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logStatusNotifierItem).noquote()
			    << "Error calling SecondaryActivate method of StatusNotifierItem"
			    << this->properties.toString();
			qCWarning(logStatusNotifierItem) << reply.error();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void StatusNotifierItem::scroll(qint32 delta, bool horizontal) {
	this->item->Scroll(delta, horizontal ? "horizontal" : "vertical");
}

void StatusNotifierItem::updateIcon() {
	this->iconIndex++;
	emit this->iconChanged();
}

DBusMenuHandle* StatusNotifierItem::menuHandle() {
	return this->menuPath.get().path().isEmpty() ? nullptr : &this->mMenuHandle;
}

void StatusNotifierItem::onMenuPathChanged() {
	this->mMenuHandle.setAddress(this->item->service(), this->menuPath.get().path());
}

void StatusNotifierItem::onGetAllFinished() {
	if (this->mReady) return;
	this->mReady = true;
	emit this->ready();
}

TrayImageHandle::TrayImageHandle(StatusNotifierItem* item)
    : QsImageHandle(QQmlImageProviderBase::Pixmap, item)
    , item(item) {}

QPixmap
TrayImageHandle::requestPixmap(const QString& /*unused*/, QSize* size, const QSize& requestedSize) {
	auto targetSize = requestedSize.isValid() ? requestedSize : QSize(100, 100);
	if (targetSize.width() == 0 || targetSize.height() == 0) targetSize = QSize(2, 2);

	auto pixmap = this->item->createPixmap(targetSize);
	if (pixmap.isNull()) {
		pixmap = IconImageProvider::missingPixmap(targetSize);
	}

	if (size != nullptr) *size = pixmap.size();
	return pixmap;
}

} // namespace qs::service::sni
