#include "trayimageprovider.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qstring.h>

#include "../../core/iconimageprovider.hpp"
#include "host.hpp"

namespace qs::service::sni {

QPixmap
TrayImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
	QPixmap pixmap;

	auto targetSize = requestedSize.isValid() ? requestedSize : QSize(100, 100);
	if (targetSize.width() == 0 || targetSize.height() == 0) targetSize = QSize(2, 2);

	auto lastSplit = id.lastIndexOf('/');
	if (lastSplit == -1) {
		qCWarning(logStatusNotifierHost) << "Invalid image request:" << id;
	} else {
		auto path = id.sliced(0, lastSplit);

		auto* item = StatusNotifierHost::instance()->itemByService(path);

		if (item == nullptr) {
			qCWarning(logStatusNotifierHost) << "Image requested for nonexistant service" << path;
		} else {
			pixmap = item->createPixmap(targetSize);
		}
	}

	if (pixmap.isNull()) {
		pixmap = IconImageProvider::missingPixmap(targetSize);
	}

	if (size != nullptr) *size = pixmap.size();
	return pixmap;
}

} // namespace qs::service::sni
