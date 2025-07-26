#include "dbusimage.hpp"

#include <qdbusargument.h>
#include <qimage.h>
#include <qloggingcategory.h>
#include <qsize.h>
#include <qsysinfo.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"

namespace qs::service::notifications {

// NOLINTNEXTLINE(misc-use-internal-linkage)
QS_DECLARE_LOGGING_CATEGORY(logNotifications); // server.cpp

QImage DBusNotificationImage::createImage() const {
	auto format = this->hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888;

	return QImage(
	    reinterpret_cast<const uchar*>(this->data.data()),
	    this->width,
	    this->height,
	    format
	);
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusNotificationImage& pixmap) {
	argument.beginStructure();
	argument >> pixmap.width;
	argument >> pixmap.height;
	auto rowstride = qdbus_cast<qint32>(argument);
	argument >> pixmap.hasAlpha;
	auto sampleBits = qdbus_cast<qint32>(argument);
	auto channels = qdbus_cast<qint32>(argument);
	argument >> pixmap.data;
	argument.endStructure();

	if (sampleBits != 8) {
		qCWarning(logNotifications) << "Unable to parse pixmap as sample count is incorrect. Got"
		                            << sampleBits << "expected" << 8;
	} else if (channels != (pixmap.hasAlpha ? 4 : 3)) {
		qCWarning(logNotifications) << "Unable to parse pixmap as channel count is incorrect."
		                            << "Got " << channels << "expected" << (pixmap.hasAlpha ? 4 : 3);
	} else if (rowstride != pixmap.width * channels) {
		qCWarning(logNotifications) << "Unable to parse pixmap as rowstride is incorrect. Got"
		                            << rowstride << "expected" << (pixmap.width * channels);
	}

	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusNotificationImage& pixmap) {
	argument.beginStructure();
	argument << pixmap.width;
	argument << pixmap.height;
	argument << pixmap.width * (pixmap.hasAlpha ? 4 : 3);
	argument << pixmap.hasAlpha;
	argument << 8;
	argument << (pixmap.hasAlpha ? 4 : 3);
	argument << pixmap.data;
	argument.endStructure();
	return argument;
}

QImage
NotificationImage::requestImage(const QString& /*unused*/, QSize* size, const QSize& /*unused*/) {
	auto image = this->image.createImage();

	if (size != nullptr) *size = image.size();
	return image;
}

} // namespace qs::service::notifications
