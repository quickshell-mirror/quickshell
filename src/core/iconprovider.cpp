#include "iconprovider.hpp"
#include <utility>

#include <qicon.h>
#include <qiconengine.h>
#include <qlogging.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>

#include "generation.hpp"

// QMenu re-calls pixmap() every time the mouse moves so its important to cache it.
class PixmapCacheIconEngine: public QIconEngine {
	void paint(
	    QPainter* /*unused*/,
	    const QRect& /*unused*/,
	    QIcon::Mode /*unused*/,
	    QIcon::State /*unused*/
	) override {
		qFatal(
		) << "Unexpected icon paint request bypassed pixmap method. Please report this as a bug.";
	}

	QPixmap pixmap(const QSize& size, QIcon::Mode /*unused*/, QIcon::State /*unused*/) override {
		if (this->lastPixmap.isNull() || size != this->lastSize) {
			this->lastPixmap = this->createPixmap(size);
			this->lastSize = size;
		}

		return this->lastPixmap;
	}

	virtual QPixmap createPixmap(const QSize& size) = 0;

private:
	QSize lastSize;
	QPixmap lastPixmap;
};

class ImageProviderIconEngine: public PixmapCacheIconEngine {
public:
	explicit ImageProviderIconEngine(QQuickImageProvider* provider, QString id)
	    : provider(provider)
	    , id(std::move(id)) {}

	QPixmap createPixmap(const QSize& size) override {
		if (this->provider->imageType() == QQmlImageProviderBase::Pixmap) {
			return this->provider->requestPixmap(this->id, nullptr, size);
		} else if (this->provider->imageType() == QQmlImageProviderBase::Image) {
			auto image = this->provider->requestImage(this->id, nullptr, size);
			return QPixmap::fromImage(image);
		} else {
			qFatal() << "Unexpected ImageProviderIconEngine image type" << this->provider->imageType();
			return QPixmap(); // never reached, satisfies lint
		}
	}

	[[nodiscard]] QIconEngine* clone() const override {
		return new ImageProviderIconEngine(this->provider, this->id);
	}

private:
	QQuickImageProvider* provider;
	QString id;
};

QIcon getEngineImageAsIcon(QQmlEngine* engine, const QUrl& url) {
	if (!engine || url.isEmpty()) return QIcon();

	auto scheme = url.scheme();
	if (scheme == "image") {
		auto providerName = url.authority();
		auto path = url.path();
		if (!path.isEmpty()) path = path.sliced(1);

		auto* provider = qobject_cast<QQuickImageProvider*>(engine->imageProvider(providerName));

		if (provider == nullptr) {
			qWarning() << "iconByUrl failed: no provider found for" << url;
			return QIcon();
		}

		if (provider->imageType() == QQmlImageProviderBase::Pixmap
		    || provider->imageType() == QQmlImageProviderBase::Image)
		{
			return QIcon(new ImageProviderIconEngine(provider, path));
		}

	} else {
		qWarning() << "iconByUrl failed: unsupported scheme" << scheme << "in path" << url;
	}

	return QIcon();
}

QIcon getCurrentEngineImageAsIcon(const QUrl& url) {
	auto* generation = EngineGeneration::currentGeneration();
	if (!generation) return QIcon();
	return getEngineImageAsIcon(generation->engine, url);
}
