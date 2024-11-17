#pragma once

#include <qpixmap.h>
#include <qquickimageprovider.h>

class IconImageProvider: public QQuickImageProvider {
public:
	explicit IconImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap) {}

	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

	static QPixmap missingPixmap(const QSize& size);

	static QString requestString(
	    const QString& icon,
	    const QString& path = QString(),
	    const QString& fallback = QString()
	);
};
