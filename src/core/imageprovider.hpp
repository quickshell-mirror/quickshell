#pragma once

#include <qimage.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

class QsImageProvider: public QQuickImageProvider {
public:
	explicit QsImageProvider(): QQuickImageProvider(QQuickImageProvider::Image) {}
	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};

class QsPixmapProvider: public QQuickImageProvider {
public:
	explicit QsPixmapProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap) {}
	QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};

class QsImageHandle: public QObject {
	Q_OBJECT;

public:
	explicit QsImageHandle(QQmlImageProviderBase::ImageType type, QObject* parent = nullptr);
	~QsImageHandle() override;
	Q_DISABLE_COPY_MOVE(QsImageHandle);

	[[nodiscard]] QString url() const;

	virtual QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize);
	virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize);

private:
	QQmlImageProviderBase::ImageType type;
	QString id;
};
