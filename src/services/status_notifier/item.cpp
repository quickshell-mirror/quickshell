#include "item.hpp"

#include <qdbuserror.h>
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
#include <qstringliteral.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/iconimageprovider.hpp"
#include "../../core/imageprovider.hpp"
#include "../../core/logcat.hpp"
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

QS_LOGGING_CATEGORY(logStatusNotifierItem, "quickshell.service.sni.item", QtWarningMsg);

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
	QObject::connect(this->item, &DBusStatusNotifierItem::NewTitle, this, [this]() {
		this->pTitle.requestUpdate();
	});

	QObject::connect(this->item, &DBusStatusNotifierItem::NewIcon, this, [this]() {
		this->pIconName.requestUpdate();
		this->pIconPixmaps.requestUpdate();
		if (this->pIconThemePath.exists()) this->pIconThemePath.requestUpdate();
	});

	QObject::connect(this->item, &DBusStatusNotifierItem::NewOverlayIcon, this, [this]() {
		this->pOverlayIconName.requestUpdate();
		this->pOverlayIconPixmaps.requestUpdate();
		if (this->pIconThemePath.exists()) this->pIconThemePath.requestUpdate();
	});

	QObject::connect(this->item, &DBusStatusNotifierItem::NewAttentionIcon, this, [this]() {
		this->pAttentionIconName.requestUpdate();
		this->pAttentionIconPixmaps.requestUpdate();
		if (this->pIconThemePath.exists()) this->pIconThemePath.requestUpdate();
	});

	QObject::connect(this->item, &DBusStatusNotifierItem::NewToolTip, this, [this]() {
		this->pTooltip.requestUpdate();
	});

	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFinished, this, &StatusNotifierItem::onGetAllFinished);
	QObject::connect(&this->properties, &DBusPropertyGroup::getAllFailed, this, &StatusNotifierItem::onGetAllFailed);
	// clang-format on

	this->bIcon.setBinding([this]() -> QString {
		if (this->bStatus.value() == Status::NeedsAttention) {
			auto name = this->bAttentionIconName.value();
			if (!name.isEmpty())
				return IconImageProvider::requestString(name, this->bIconThemePath.value());
		} else {
			auto name = this->bIconName.value();
			auto overlayName = this->bOverlayIconName.value();
			if (!name.isEmpty() && overlayName.isEmpty())
				return IconImageProvider::requestString(name, this->bIconThemePath.value());
		}

		return this->imageHandle.url() % "/" % QString::number(this->pixmapIndex);
	});

	this->bHasMenu.setBinding([this]() { return !this->bMenuPath.value().path().isEmpty(); });

	QObject::connect(
	    this->item,
	    &DBusStatusNotifierItem::NewStatus,
	    this,
	    [this](const QString& value) {
		    auto result = DBusDataTransform<Status::Enum>::fromWire(value);

		    if (result.isValid()) {
			    this->bStatus = result.value;
			    qCDebug(logStatusNotifierItem)
			        << "Received status update for" << this->properties.toString() << result.value;
		    } else {
			    qCWarning(logStatusNotifierItem)
			        << "Received invalid status update for" << this->properties.toString() << value;
		    }
	    }
	);

	this->properties.setInterface(this->item);
	this->properties.updateAllViaGetAll();
}

bool StatusNotifierItem::isValid() const { return this->item->isValid(); }
bool StatusNotifierItem::isReady() const { return this->mReady; }

QPixmap StatusNotifierItem::createPixmap(const QSize& size) const {
	auto needsAttention = this->bStatus.value() == Status::NeedsAttention;

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
		if (!this->bAttentionIconName.value().isEmpty()) {
			auto icon = QIcon::fromTheme(this->bAttentionIconName.value());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->bAttentionIconPixmaps.value());

			if (icon == nullptr) {
				icon = closestPixmap(size, this->bIconPixmaps.value());
			}

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				pixmap = QPixmap::fromImage(image);
			}
		}
	} else {
		if (!this->bIconName.value().isEmpty()) {
			auto icon = QIcon::fromTheme(this->bIconName.value());
			pixmap = icon.pixmap(size.width(), size.height());
		} else {
			const auto* icon = closestPixmap(size, this->bIconPixmaps.value());

			if (icon != nullptr) {
				const auto image =
				    icon->createImage().scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				pixmap = QPixmap::fromImage(image);
			}
		}

		QPixmap overlay;
		if (!this->bOverlayIconName.value().isEmpty()) {
			auto icon = QIcon::fromTheme(this->bOverlayIconName.value());
			overlay = icon.pixmap(pixmap.width(), pixmap.height());
		} else {
			const auto* icon = closestPixmap(pixmap.size(), this->bOverlayIconPixmaps.value());

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
			if (reply.error().type() == QDBusError::UnknownMethod) {
				qCDebug(logStatusNotifierItem) << "Tried to call Activate method of StatusNotifierItem"
				                               << this->properties.toString() << "but it does not exist.";
			} else {
				qCWarning(logStatusNotifierItem).noquote()
				    << "Error calling Activate method of StatusNotifierItem" << this->properties.toString();
				qCWarning(logStatusNotifierItem) << reply.error();
			}
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
			if (reply.error().type() == QDBusError::UnknownMethod) {
				qCDebug(logStatusNotifierItem)
				    << "Tried to call SecondaryActivate method of StatusNotifierItem"
				    << this->properties.toString() << "but it does not exist.";
			} else {
				qCWarning(logStatusNotifierItem).noquote()
				    << "Error calling SecondaryActivate method of StatusNotifierItem"
				    << this->properties.toString();
				qCWarning(logStatusNotifierItem) << reply.error();
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void StatusNotifierItem::scroll(qint32 delta, bool horizontal) const {
	this->item->Scroll(delta, horizontal ? "horizontal" : "vertical");
}

void StatusNotifierItem::updatePixmapIndex() { this->pixmapIndex = this->pixmapIndex + 1; }

DBusMenuHandle* StatusNotifierItem::menuHandle() {
	return this->bMenuPath.value().path().isEmpty() ? nullptr : &this->mMenuHandle;
}

void StatusNotifierItem::onMenuPathChanged() {
	this->mMenuHandle.setAddress(this->item->service(), this->bMenuPath.value().path());
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
    : QsImageHandle(QQmlImageProviderBase::Pixmap)
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

namespace qs::dbus {
using namespace qs::service::sni;

DBusResult<Status::Enum> DBusDataTransform<Status::Enum>::fromWire(const QString& wire) {
	if (wire == QStringLiteral("Passive")) return DBusResult(Status::Passive);
	if (wire == QStringLiteral("Active")) return DBusResult(Status::Active);
	if (wire == QStringLiteral("NeedsAttention")) return DBusResult(Status::NeedsAttention);

	return DBusResult<Status::Enum>(QDBusError(
	    QDBusError::InvalidArgs,
	    QString("Nonconformant StatusNotifierItem Status: %1").arg(wire)
	));
}

DBusResult<Category::Enum> DBusDataTransform<Category::Enum>::fromWire(const QString& wire) {
	if (wire == "ApplicationStatus") return DBusResult(Category::ApplicationStatus);
	if (wire == "Communications") return DBusResult(Category::Communications);
	if (wire == "SystemServices") return DBusResult(Category::SystemServices);
	if (wire == "Hardware") return DBusResult(Category::Hardware);

	return DBusResult<Category::Enum>(QDBusError(
	    QDBusError::InvalidArgs,
	    QString("Nonconformant StatusNotifierItem Category: %1").arg(wire)
	));
}

} // namespace qs::dbus
