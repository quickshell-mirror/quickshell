#pragma once

#include <qobject.h>
#include <qquickitem.h>
#include <qsize.h>

/// Allows for saving an item grab to an file asynchronously.
class ItemImageGrab: public QObject {
	Q_OBJECT;
	QML_ELEMENT;

public:
	explicit ItemImageGrab(QObject* parent = nullptr): QObject(parent) {};

	Q_INVOKABLE void grab(QQuickItem* target, const QUrl& path);
	Q_INVOKABLE void grab(QQuickItem* target, const QUrl& path, const QSize& targetSize);

signals:
	void saved(const QUrl& path);
	void failed(const QUrl& path);
};
