#pragma once

#include <qdebug.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlinfo.h>
#include <qqmlintegration.h>
#include <qscreen.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

// unfortunately QQuickScreenInfo is private.

/// Monitor object useful for setting the monitor for a @@QsWindow
/// or querying information about the monitor.
///
/// > [!WARNING] If the monitor is disconnected than any stored copies of its ShellMonitor will
/// > be marked as dangling and all properties will return default values.
/// > Reconnecting the monitor will not reconnect it to the ShellMonitor object.
///
/// Due to some technical limitations, it was not possible to reuse the native qml @@QtQuick.Screen type.
class QuickshellScreenInfo: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(ShellScreen);
	QML_UNCREATABLE("ShellScreen can only be obtained via Quickshell.screens");
	// clang-format off
	/// The name of the screen as seen by the operating system.
	///
	/// Usually something like `DP-1`, `HDMI-1`, `eDP-1`.
	Q_PROPERTY(QString name READ name CONSTANT);
	/// The model of the screen as seen by the operating system.
	Q_PROPERTY(QString model READ model CONSTANT);
	/// The serial number of the screen as seen by the operating system.
	Q_PROPERTY(QString serialNumber READ serialNumber CONSTANT);
	Q_PROPERTY(qint32 x READ x NOTIFY geometryChanged);
	Q_PROPERTY(qint32 y READ y NOTIFY geometryChanged);
	Q_PROPERTY(qint32 width READ width NOTIFY geometryChanged);
	Q_PROPERTY(qint32 height READ height NOTIFY geometryChanged);
	/// The number of physical pixels per millimeter.
	Q_PROPERTY(qreal physicalPixelDensity READ physicalPixelDensity NOTIFY physicalPixelDensityChanged);
	/// The number of device-independent (scaled) pixels per millimeter.
	Q_PROPERTY(qreal logicalPixelDensity READ logicalPixelDensity NOTIFY logicalPixelDensityChanged);
	/// The ratio between physical pixels and device-independent (scaled) pixels.
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY physicalPixelDensityChanged);
	Q_PROPERTY(Qt::ScreenOrientation orientation READ orientation NOTIFY orientationChanged);
	Q_PROPERTY(Qt::ScreenOrientation primaryOrientation READ primaryOrientation NOTIFY primaryOrientationChanged);
	// clang-format on

public:
	QuickshellScreenInfo(QObject* parent, QScreen* screen); //: QObject(parent), screen(screen) {}

	bool operator==(QuickshellScreenInfo& other) const;

	[[nodiscard]] QString name() const;
	[[nodiscard]] QString model() const;
	[[nodiscard]] QString serialNumber() const;
	[[nodiscard]] qint32 x() const;
	[[nodiscard]] qint32 y() const;
	[[nodiscard]] qint32 width() const;
	[[nodiscard]] qint32 height() const;
	[[nodiscard]] qreal physicalPixelDensity() const;
	[[nodiscard]] qreal logicalPixelDensity() const;
	[[nodiscard]] qreal devicePixelRatio() const;
	[[nodiscard]] Qt::ScreenOrientation orientation() const;
	[[nodiscard]] Qt::ScreenOrientation primaryOrientation() const;

	[[nodiscard]] Q_INVOKABLE QString toString() const;

	QScreen* screen;

private:
	void warnDangling() const;
	bool dangling = false;

signals:
	void geometryChanged();
	void physicalPixelDensityChanged();
	void logicalPixelDensityChanged();
	void orientationChanged();
	void primaryOrientationChanged();

private slots:
	void screenDestroyed();
};

QDebug operator<<(QDebug debug, const QuickshellScreenInfo* screen);
