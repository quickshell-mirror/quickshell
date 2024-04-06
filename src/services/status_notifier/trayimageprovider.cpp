#include "trayimageprovider.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qstring.h>

#include "host.hpp"

namespace qs::service::sni {

QPixmap
TrayImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
	auto split = id.split('/');
	if (split.size() != 2) {
		qCWarning(logStatusNotifierHost) << "Invalid image request:" << id;
		return QPixmap();
	}

	auto* item = StatusNotifierHost::instance()->itemByService(split[0]);

	if (item == nullptr) {
		qCWarning(logStatusNotifierHost) << "Image requested for nonexistant service" << split[0];
		return QPixmap();
	}

	auto pixmap = item->createPixmap(requestedSize);
	if (size != nullptr) *size = pixmap.size();
	return pixmap;
}

} // namespace qs::service::sni
