#include "itemimagegrab.hpp"

#include <qobject.h>
#include <qquickitem.h>
#include <qquickitemgrabresult.h>
#include <qsize.h>
#include <qthreadpool.h>
#include <qtmetamacros.h>

void ItemImageGrab::grab(QQuickItem* target, const QUrl& path) {
	this->grab(target, path, QSize());
}

void ItemImageGrab::grab(QQuickItem* target, const QUrl& path, const QSize& targetSize) {
	this->cropAndGrab(target, path, QRect(), targetSize);
}

void ItemImageGrab::cropAndGrab(QQuickItem* target, const QUrl& path, const QRect& rect) {
	this->cropAndGrab(target, path, rect, QSize());
}

void ItemImageGrab::cropAndGrab(
    QQuickItem* target,
    const QUrl& path,
    const QRect& rect,
    const QSize& targetSize
) {
	if (!target) {
		qWarning() << "ItemImageGrab: a target is required";
		return;
	}

	if (!path.isLocalFile()) {
		qWarning() << "ItemImageGrab: can only save to a file on the local filesystem";
		return;
	}

	QSharedPointer<QQuickItemGrabResult> grabResult;
	if (targetSize.isEmpty()) {
		grabResult = target->grabToImage();
	} else {
		grabResult = target->grabToImage(targetSize);
	}

	QObject::connect(
	    grabResult.data(),
	    &QQuickItemGrabResult::ready,
	    this,
	    [grabResult, rect, path, this]() {
		    QThreadPool::globalInstance()->start([grabResult, rect, path, this] {
			    QImage image = grabResult->image();

			    if (!rect.isEmpty()) {
				    image = image.copy(rect);
			    }

			    const QString localFile = path.toLocalFile();

			    if (image.save(localFile)) {
				    emit this->saved(localFile);
			    } else {
				    emit this->failed(path);
			    }
		    });
	    }
	);
}
