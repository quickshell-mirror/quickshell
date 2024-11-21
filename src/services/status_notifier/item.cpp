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
#include "../../core/platformmenu.hpp"
#include "../../dbus/dbusmenu/dbusmenu.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_item.h"
#include "dbus_item_types.hpp"
#include "host.hpp"
#include "watcher.hpp"

using namespace qs::dbus;
using namespace qs::dbus::dbusmenu;
using namespace qs::menu::platform;

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
	QObject::connect(this->item, &DBusStatusNotifierItem::NewTitle, &this->pTitle, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->pIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->pIconPixmaps, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, &this->pIconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->pOverlayIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->pOverlayIconPixmaps, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, &this->pIconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->pAttentionIconName, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->pAttentionIconPixmaps, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, &this->pIconThemePath, &AbstractDBusProperty::update);
	QObject::connect(this->item, &DBusStatusNotifierItem::NewToolTip, &this->pTooltip, &AbstractDBusProperty::update);

	QObject::connect(&this->pIconThemePath, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->pIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->pAttentionIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->pOverlayIconName, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->pIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->pAttentionIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);
	QObject::connect(&this->pOverlayIconPixmaps, &AbstractDBusProperty::changed, this, &StatusNotifierItem::updateIcon);

	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFinished, this, &StatusNotifierItem::onGetAllFinished);
	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFailed, this, &StatusNotifierItem::onGetAllFailed);
	QObject::connect(&this->pMenuPath, &AbstractDBusProperty::changed, this, &StatusNotifierItem::onMenuPathChanged);
	// clang-format on

	QObject::connect(this->item, &DBusStatusNotifierItem::NewStatus, this, [this](QString value) {
		qCDebug(logStatusNotifierItem) << "Received update for" << this->pStatus.toString() << value;
		this->pStatus.set(std::move(value));
	});

	this->properties.setInterface(this->item);
	this->properties.updateAllViaGetAll();
}

bool StatusNotifierItem::isValid() const { return this->item->isValid(); }
bool StatusNotifierItem::isReady() const { return this->mReady; }

QString StatusNotifierItem::iconId() const {
	if (this->pStatus.get() == "NeedsAttention") {
		auto name = this->pAttentionIconName.get();
		if (!name.isEmpty()) return IconImageProvider::requestString(name, this->pIconThemePath.get());
	} else {
		auto name = this->pIconName.get();
		auto overlayName = this->pOverlayIconName.get();
		if (!name.isEmpty() && overlayName.isEmpty())
			return IconImageProvider::requestString(name, this->pIconThemePath.get());
	}

	return this->imageHandle.url() + "/" + QString::number(this->iconIndex);
}

QPixmap StatusNotifierItem::createPixmap(const QSize& size) const {
	auto needsAttention = this->pStatus.get() == "NeedsAttention";

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
		if (!this->pAttentionIconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->pAttentionIconName.get());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->pAttentionIconPixmaps.get());

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				pixmap = QPixmap::fromImage(image);
			}
		}
	} else {
		if (!this->pIconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->pIconName.get());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->pIconPixmaps.get());

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				pixmap = QPixmap::fromImage(image);
			}
		}

		QPixmap overlay;
		if (!this->pOverlayIconName.get().isEmpty()) {
			auto icon = QIcon::fromTheme(this->pOverlayIconName.get());
			overlay = icon.pixmap(pixmap.width(), pixmap.height());
		} else {
			const auto* icon = closestPixmap(pixmap.size(), this->pOverlayIconPixmaps.get());

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

void StatusNotifierItem::scroll(qint32 delta, bool horizontal) const {
	this->item->Scroll(delta, horizontal ? "horizontal" : "vertical");
}

void StatusNotifierItem::updateIcon() {
	this->iconIndex++;
	emit this->iconChanged();
}

DBusMenuHandle* StatusNotifierItem::menuHandle() {
	return this->pMenuPath.get().path().isEmpty() ? nullptr : &this->mMenuHandle;
}

void StatusNotifierItem::onMenuPathChanged() {
	this->mMenuHandle.setAddress(this->item->service(), this->pMenuPath.get().path());
}

void StatusNotifierItem::onGetAllFinished() {
	if (this->mReady) return;
	this->mReady = true;
	emit this->ready();
}

void StatusNotifierItem::onGetAllFailed() const {
	// Not changing the item to ready, as it is almost definitely broken.
	if (!this->mReady) {
		qWarning(logStatusNotifierItem) << "Failed to load tray item" << this->properties.toString();

		if (!StatusNotifierWatcher::instance()->isRegistered()) {
			qWarning(logStatusNotifierItem)
			    << "Another StatusNotifier host seems to be running. Please disable it and check that "
			       "the problem persists before reporting an issue.";
		}
	}
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
		qCWarning(logStatusNotifierItem) << "Unable to create pixmap for tray icon" << this->item;
		pixmap = IconImageProvider::missingPixmap(targetSize);
	}

	if (size != nullptr) *size = pixmap.size();
	return pixmap;
}

QString StatusNotifierItem::id() const { return this->pId.get(); }
QString StatusNotifierItem::title() const { return this->pTitle.get(); }

Status::Enum StatusNotifierItem::status() const {
	auto status = this->pStatus.get();

	if (status == "Passive") return Status::Passive;
	if (status == "Active") return Status::Active;
	if (status == "NeedsAttention") return Status::NeedsAttention;

	qCWarning(logStatusNotifierItem) << "Nonconformant StatusNotifierItem status" << status
	                                 << "returned for" << this->properties.toString();

	return Status::Passive;
}

Category::Enum StatusNotifierItem::category() const {
	auto category = this->pCategory.get();

	if (category == "ApplicationStatus") return Category::ApplicationStatus;
	if (category == "SystemServices") return Category::SystemServices;
	if (category == "Hardware") return Category::Hardware;

	qCWarning(logStatusNotifierItem) << "Nonconformant StatusNotifierItem category" << category
	                                 << "returned for" << this->properties.toString();

	return Category::ApplicationStatus;
}

QString StatusNotifierItem::tooltipTitle() const { return this->pTooltip.get().title; }
QString StatusNotifierItem::tooltipDescription() const { return this->pTooltip.get().description; }

bool StatusNotifierItem::hasMenu() const { return !this->pMenuPath.get().path().isEmpty(); }
bool StatusNotifierItem::onlyMenu() const { return this->pIsMenu.get(); }

void StatusNotifierItem::display(QObject* parentWindow, qint32 relativeX, qint32 relativeY) {
	if (!this->menuHandle()) {
		qCritical() << "No menu present for" << this;
		return;
	}

	auto* handle = this->menuHandle();

	auto onMenuChanged = [this, parentWindow, relativeX, relativeY, handle]() {
		QObject::disconnect(handle, nullptr, this, nullptr);

		if (!handle->menu()) {
			handle->unrefHandle();
			return;
		}

		auto* platform = new PlatformMenuEntry(handle->menu());

		// clang-format off
		QObject::connect(platform, &PlatformMenuEntry::closed, this, [=]() { platform->deleteLater(); });
		QObject::connect(platform, &QObject::destroyed, this, [=]() { handle->unrefHandle(); });
		// clang-format on

		auto success = platform->display(parentWindow, relativeX, relativeY);

		// calls destroy which also unrefs
		if (!success) delete platform;
	};

	if (handle->menu()) onMenuChanged();
	QObject::connect(handle, &DBusMenuHandle::menuChanged, this, onMenuChanged);
	handle->refHandle();
}

} // namespace qs::service::sni
