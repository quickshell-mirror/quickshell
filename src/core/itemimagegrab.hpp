#pragma once

#include <qobject.h>
#include <qquickitem.h>
#include <qsize.h>

/// Allows for saving an item grab to an file asynchronously.
class ItemImageGrab: public QObject {
	Q_OBJECT;

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged);
    QML_ELEMENT;

public:
	explicit ItemImageGrab(QObject* parent = nullptr): QObject(parent) {};

	[[nodiscard]] QQuickItem* target() const;
	void setTarget(QQuickItem* target);

    Q_INVOKABLE void grab(const QUrl& path);
    Q_INVOKABLE void grab(const QUrl& path, const QSize& targetSize);
    Q_INVOKABLE void grab(QQuickItem* target, const QUrl& path);
    Q_INVOKABLE void grab(QQuickItem* target, const QUrl& path, const QSize& targetSize);

signals:
    void saved(const QUrl& path);
    void failed(const QUrl& path);

	void targetChanged();

private:
	QPointer<QQuickItem> mTarget;
};
