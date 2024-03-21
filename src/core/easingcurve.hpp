#pragma once

#include <qeasingcurve.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmlintegration.h>
#include <qrect.h>
#include <qtmetamacros.h>

///! Easing curve.
/// Directly accessible easing curve as used in property animations.
class EasingCurve: public QObject {
	Q_OBJECT;
	/// Easing curve settings. Works exactly the same as
	/// [PropertyAnimation.easing](https://doc.qt.io/qt-6/qml-qtquick-propertyanimation.html#easing-prop).
	Q_PROPERTY(QEasingCurve curve READ curve WRITE setCurve NOTIFY curveChanged);
	QML_ELEMENT;

public:
	EasingCurve(QObject* parent = nullptr): QObject(parent) {}

	/// Returns the Y value for the given X value on the curve
	/// from 0.0 to 1.0.
	Q_INVOKABLE [[nodiscard]] qreal valueAt(qreal x) const;
	/// Interpolates between two values using the given X coordinate.
	Q_INVOKABLE [[nodiscard]] qreal interpolate(qreal x, qreal a, qreal b) const;
	/// Interpolates between two points using the given X coordinate.
	Q_INVOKABLE [[nodiscard]] QPointF interpolate(qreal x, const QPointF& a, const QPointF& b) const;
	/// Interpolates two rects using the given X coordinate.
	Q_INVOKABLE [[nodiscard]] QRectF interpolate(qreal x, const QRectF& a, const QRectF& b) const;

	[[nodiscard]] QEasingCurve curve() const;
	void setCurve(QEasingCurve curve);

signals:
	void curveChanged();

private:
	QEasingCurve mCurve;
};
