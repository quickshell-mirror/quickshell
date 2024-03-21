#include "easingcurve.hpp"
#include <utility>

#include <qeasingcurve.h>
#include <qpoint.h>
#include <qrect.h>
#include <qtmetamacros.h>
#include <qtypes.h>

qreal EasingCurve::valueAt(qreal x) const { return this->mCurve.valueForProgress(x); }

qreal EasingCurve::interpolate(qreal x, qreal a, qreal b) const {
	return a + (b - a) * this->valueAt(x);
}

QPointF EasingCurve::interpolate(qreal x, const QPointF& a, const QPointF& b) const {
	return QPointF(this->interpolate(x, a.x(), b.x()), this->interpolate(x, a.y(), b.y()));
}

QRectF EasingCurve::interpolate(qreal x, const QRectF& a, const QRectF& b) const {
	return QRectF(
	    this->interpolate(x, a.topLeft(), b.topLeft()),
	    this->interpolate(x, a.bottomRight(), b.bottomRight())
	);
}

QEasingCurve EasingCurve::curve() const { return this->mCurve; }

void EasingCurve::setCurve(QEasingCurve curve) {
	if (this->mCurve == curve) return;
	this->mCurve = std::move(curve);
	emit this->curveChanged();
}
