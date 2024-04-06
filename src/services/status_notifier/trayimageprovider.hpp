#pragma once

#include <qpixmap.h>
#include <qquickimageprovider.h>
#include <qtmetamacros.h>

namespace qs::service::sni {

class TrayImageProvider: public QQuickImageProvider {
public:
	explicit TrayImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap) {}

	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};

} // namespace qs::service::sni
