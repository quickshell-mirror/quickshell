#include "itemimagegrab.hpp"

#include <qobject.h>
#include <qquickitem.h>
#include <qquickitemgrabresult.h>
#include <qsize.h>
#include <qthreadpool.h>
#include <qtmetamacros.h>

QQuickItem* ItemImageGrab::target() const { return this->mTarget; }

void ItemImageGrab::setTarget(QQuickItem* target) {
    if (target == this->mTarget) return;

    this->mTarget = target;
    emit this->targetChanged();
}

void ItemImageGrab::grab(const QUrl& path) { this->grab(this->mTarget, path); }

void ItemImageGrab::grab(const QUrl& path, const QSize& targetSize) { this->grab(this->mTarget, path, targetSize); }

void ItemImageGrab::grab(QQuickItem* target, const QUrl& path) { this->grab(target, path, QSize()); }

void ItemImageGrab::grab(QQuickItem* target, const QUrl& path, const QSize& targetSize) {
    if (!target) return;

    QSharedPointer<QQuickItemGrabResult> grabResult;
    if (targetSize.isEmpty()){
        grabResult = target->grabToImage();
    }else{
        grabResult = target->grabToImage(targetSize);}

    QObject::connect(grabResult.data(), &QQuickItemGrabResult::ready, this, [grabResult, path, this]() {
        QThreadPool::globalInstance()->start([grabResult, path, this] {
            if (grabResult->saveToFile(path)) {
                emit this->saved(path);
            } else {
                emit this->failed(path);
            }
        });
    });
}
