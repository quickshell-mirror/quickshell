#include "imageprovider.hpp"

#include <qdebug.h>
#include <qimage.h>
#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qqmlengine.h>

static QMap<QString, QsImageHandle*> liveImages; // NOLINT

QsImageHandle::QsImageHandle(QQmlImageProviderBase::ImageType type, QObject* parent)
    : QObject(parent)
    , type(type) {
	{
		auto dbg = QDebug(&this->id);
		dbg.nospace() << static_cast<void*>(this);
	}

	liveImages.insert(this->id, this);
}

QsImageHandle::~QsImageHandle() { liveImages.remove(this->id); }

QString QsImageHandle::url() const {
	QString url = "image://";
	if (this->type == QQmlImageProviderBase::Image) url += "qsimage";
	else if (this->type == QQmlImageProviderBase::Pixmap) url += "qspixmap";
	url += "/" + this->id;
	return url;
}

QImage
QsImageHandle::requestImage(const QString& /*unused*/, QSize* /*unused*/, const QSize& /*unused*/) {
	qWarning() << "Image handle" << this << "does not provide QImages";
	return QImage();
}

QPixmap QsImageHandle::
    requestPixmap(const QString& /*unused*/, QSize* /*unused*/, const QSize& /*unused*/) {
	qWarning() << "Image handle" << this << "does not provide QPixmaps";
	return QPixmap();
}

void parseReq(const QString& req, QString& target, QString& param) {
	auto splitIdx = req.indexOf('/');
	if (splitIdx != -1) {
		target = req.sliced(0, splitIdx);
		param = req.sliced(splitIdx + 1);
	} else {
		target = req;
	}
}

QImage QsImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
	QString target;
	QString param;
	parseReq(id, target, param);

	auto* handle = liveImages.value(target);
	if (handle != nullptr) {
		return handle->requestImage(param, size, requestedSize);
	} else {
		qWarning() << "Requested image from unknown handle" << id;
		return QImage();
	}
}

QPixmap
QsPixmapProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
	QString target;
	QString param;
	parseReq(id, target, param);

	auto* handle = liveImages.value(target);
	if (handle != nullptr) {
		return handle->requestPixmap(param, size, requestedSize);
	} else {
		qWarning() << "Requested image from unknown handle" << id;
		return QPixmap();
	}
}
