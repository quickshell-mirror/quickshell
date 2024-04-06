#include "iconimageprovider.hpp"

#include <qicon.h>
#include <qlogging.h>
#include <qpixmap.h>

QPixmap
IconImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
	auto icon = QIcon::fromTheme(id);

	auto targetSize = requestedSize.isValid() ? requestedSize : QSize(100, 100);
	auto pixmap = icon.pixmap(targetSize.width(), targetSize.height());

	if (size != nullptr) *size = pixmap.size();
	return pixmap;
}
