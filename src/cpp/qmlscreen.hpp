#pragma once

#include <qnamespace.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>

// unfortunately QQuickScreenInfo is private.
class QuickShellScreenInfo: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("QuickShellScreenInfo can only be obtained via QuickShell.screens");
	// clang-format off
	Q_PROPERTY(QString name READ name NOTIFY nameChanged);
	Q_PROPERTY(qint32 width READ width NOTIFY widthChanged);
	Q_PROPERTY(qint32 height READ height NOTIFY heightChanged);
	Q_PROPERTY(qreal pixelDensity READ pixelDensity NOTIFY logicalPixelDensityChanged);
	Q_PROPERTY(qreal logicalPixelDensity READ logicalPixelDensity NOTIFY logicalPixelDensityChanged);
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged);
	Q_PROPERTY(Qt::ScreenOrientation orientation READ orientation NOTIFY orientationChanged);
	Q_PROPERTY(Qt::ScreenOrientation primatyOrientation READ primaryOrientation NOTIFY primaryOrientationChanged);
	// clang-format on

public:
	QuickShellScreenInfo(QObject* parent, QScreen* screen): QObject(parent), screen(screen) {}

	bool operator==(QuickShellScreenInfo& other) const;

	[[nodiscard]] QString name() const;
	[[nodiscard]] qint32 width() const;
	[[nodiscard]] qint32 height() const;
	[[nodiscard]] qreal pixelDensity() const;
	[[nodiscard]] qreal logicalPixelDensity() const;
	[[nodiscard]] qreal devicePixelRatio() const;
	[[nodiscard]] Qt::ScreenOrientation orientation() const;
	[[nodiscard]] Qt::ScreenOrientation primaryOrientation() const;

	QScreen* screen;

signals:
	void nameChanged();
	void widthChanged();
	void heightChanged();
	void pixelDensityChanged();
	void logicalPixelDensityChanged();
	void devicePixelRatioChanged();
	void orientationChanged();
	void primaryOrientationChanged();
};
