#pragma once

#include <utility>

#include <qdbusargument.h>
#include <qimage.h>
#include <qobject.h>

#include "../../core/imageprovider.hpp"

namespace qs::service::notifications {

struct DBusNotificationImage {
	qint32 width = 0;
	qint32 height = 0;
	bool hasAlpha = false;
	QByteArray data;

	// valid only for the lifetime of the pixmap
	[[nodiscard]] QImage createImage() const;
};

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusNotificationImage& pixmap);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusNotificationImage& pixmap);

class NotificationImage: public QsImageHandle {
public:
	explicit NotificationImage(DBusNotificationImage image, QObject* parent)
	    : QsImageHandle(QQuickAsyncImageProvider::Image, parent)
	    , image(std::move(image)) {}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

	DBusNotificationImage image;
};
} // namespace qs::service::notifications
