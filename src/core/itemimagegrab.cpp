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
	if (!target) {
		qWarning() << "ItemImageGrab::grab: a target is required";
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
	    [grabResult, path, this]() {
		    QThreadPool::globalInstance()->start([grabResult, path, this] {
			    if (grabResult->saveToFile(path)) {
				    emit this->saved(path);
			    } else {
				    emit this->failed(path);
			    }
		    });
	    }
	);
}
