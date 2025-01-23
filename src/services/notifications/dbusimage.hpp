#pragma once

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

class NotificationImage: public QsIndexedImageHandle {
public:
	explicit NotificationImage(): QsIndexedImageHandle(QQuickAsyncImageProvider::Image) {}

	[[nodiscard]] bool hasData() const { return !this->image.data.isEmpty(); }
	void clear() { this->image.data.clear(); }

	[[nodiscard]] DBusNotificationImage& writeImage() {
		this->imageChanged();
		return this->image;
	}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
	DBusNotificationImage image;
};

} // namespace qs::service::notifications
