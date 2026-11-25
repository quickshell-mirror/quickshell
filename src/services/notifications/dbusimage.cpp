#include "dbusimage.hpp"
#include <limits>

#include <qdbusargument.h>
#include <qimage.h>
#include <qloggingcategory.h>
#include <qsize.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"

namespace qs::service::notifications {

// NOLINTNEXTLINE(misc-use-internal-linkage)
QS_DECLARE_LOGGING_CATEGORY(logNotifications); // server.cpp

void DBusNotificationImage::clear() {
	this->width = 0;
	this->height = 0;
	this->rowstride = 0;
	this->hasAlpha = false;
	this->data.clear();
}

QImage DBusNotificationImage::createImage() const {
	if (this->data.isEmpty() || this->width <= 0 || this->height <= 0 || this->rowstride <= 0) {
		return QImage();
	}

	auto minRowBytes = static_cast<qint64>(this->width) * (this->hasAlpha ? 4 : 3);
	if (this->rowstride < minRowBytes) return QImage();

	// The last row may omit trailing rowstride padding (per GdkPixbuf / Notification Spec),
	// whereas QImage assumes every row spans bytesPerLine bytes (e.g. during copy()).
	auto bytesPerLine = this->height == 1 ? minRowBytes : static_cast<qint64>(this->rowstride);
	auto requiredBytes = bytesPerLine * static_cast<qint64>(this->height);

	if (this->data.size() < requiredBytes) return QImage();

	auto format = this->hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888;

	return QImage(
	    reinterpret_cast<const uchar*>(this->data.constData()),
	    this->width,
	    this->height,
	    static_cast<qsizetype>(bytesPerLine),
	    format
	);
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusNotificationImage& pixmap) {
	pixmap.clear();

	qint32 sampleBits = 0;
	qint32 channels = 0;

	argument.beginStructure();
	argument >> pixmap.width;
	argument >> pixmap.height;
	argument >> pixmap.rowstride;
	argument >> pixmap.hasAlpha;
	argument >> sampleBits;
	argument >> channels;
	argument >> pixmap.data;
	argument.endStructure();

	auto expectedChannels = pixmap.hasAlpha ? 4 : 3;

	if (sampleBits != 8) {
		qCWarning(logNotifications) << "Unable to parse pixmap as sample count is incorrect. Got"
		                            << sampleBits << "expected" << 8;
		pixmap.clear();
		return argument;
	}

	if (channels != expectedChannels) {
		qCWarning(logNotifications) << "Unable to parse pixmap as channel count is incorrect. Got"
		                            << channels << "expected" << expectedChannels;
		pixmap.clear();
		return argument;
	}

	auto minRowBytes = static_cast<qint64>(pixmap.width) * expectedChannels;

	if (pixmap.width <= 0 || pixmap.height <= 0 || pixmap.rowstride < minRowBytes) {
		qCWarning(logNotifications) << "Unable to parse pixmap as dimensions or rowstride are invalid."
		                            << "Got" << pixmap.width << 'x' << pixmap.height << "rowstride"
		                            << pixmap.rowstride;
		pixmap.clear();
		return argument;
	}

	auto minBytes = static_cast<qint64>(pixmap.rowstride) * (static_cast<qint64>(pixmap.height) - 1)
	              + minRowBytes;

	if (pixmap.data.size() < minBytes) {
		qCWarning(logNotifications) << "Unable to parse pixmap as data size is too small. Got"
		                            << pixmap.data.size() << "bytes, expected at least" << minBytes
		                            << "for" << pixmap.width << 'x' << pixmap.height;
		pixmap.clear();
		return argument;
	}

	// QImage reads bytesPerLine * height bytes on deep copies even if the last row has no padding.
	if (pixmap.height > 1) {
		auto paddedBytes = static_cast<qint64>(pixmap.rowstride) * static_cast<qint64>(pixmap.height);

		if (pixmap.data.size() < paddedBytes) {
			if (paddedBytes > std::numeric_limits<qsizetype>::max()) {
				qCWarning(logNotifications)
				    << "Unable to parse pixmap as padded size overflows."
				    << "Got" << pixmap.width << 'x' << pixmap.height << "rowstride" << pixmap.rowstride;
				pixmap.clear();
				return argument;
			}

			pixmap.data.resize(static_cast<qsizetype>(paddedBytes), '\0');
		}
	}

	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusNotificationImage& pixmap) {
	argument.beginStructure();
	argument << pixmap.width;
	argument << pixmap.height;
	argument << pixmap.rowstride;
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
