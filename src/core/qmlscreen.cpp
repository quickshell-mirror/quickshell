#include "qmlscreen.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qscreen.h>
#include <qtypes.h>

QuickshellScreenInfo::QuickshellScreenInfo(QObject* parent, QScreen* screen)
    : QObject(parent)
    , screen(screen) {

	if (this->screen != nullptr) {
		// clang-format off
		QObject::connect(this->screen, &QScreen::geometryChanged, this, &QuickshellScreenInfo::geometryChanged);
		QObject::connect(this->screen, &QScreen::physicalDotsPerInchChanged, this, &QuickshellScreenInfo::physicalPixelDensityChanged);
		QObject::connect(this->screen, &QScreen::logicalDotsPerInchChanged, this, &QuickshellScreenInfo::logicalPixelDensityChanged);
		QObject::connect(this->screen, &QScreen::orientationChanged, this, &QuickshellScreenInfo::orientationChanged);
		QObject::connect(this->screen, &QScreen::primaryOrientationChanged, this, &QuickshellScreenInfo::primaryOrientationChanged);
		QObject::connect(this->screen, &QObject::destroyed, this, &QuickshellScreenInfo::screenDestroyed);
		// clang-format on
	}
}

bool QuickshellScreenInfo::operator==(QuickshellScreenInfo& other) const {
	return this->screen == other.screen;
}

void QuickshellScreenInfo::warnDangling() const {
	if (this->dangling) {
		qWarning() << "attempted to use dangling screen object";
	}
}

QString QuickshellScreenInfo::name() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return "{ NULL SCREEN }";
	}

	return this->screen->name();
}

QString QuickshellScreenInfo::model() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return "{ NULL SCREEN }";
	}

	return this->screen->model();
}

QString QuickshellScreenInfo::serialNumber() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return "{ NULL SCREEN }";
	}

	return this->screen->serialNumber();
}

qint32 QuickshellScreenInfo::x() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0;
	}

	return this->screen->geometry().x();
}

qint32 QuickshellScreenInfo::y() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0;
	}

	return this->screen->geometry().y();
}

qint32 QuickshellScreenInfo::width() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0;
	}

	return this->screen->size().width();
}

qint32 QuickshellScreenInfo::height() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0;
	}

	return this->screen->size().height();
}

qreal QuickshellScreenInfo::physicalPixelDensity() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0.0;
	}

	return this->screen->physicalDotsPerInch() / 25.4;
}

qreal QuickshellScreenInfo::logicalPixelDensity() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0.0;
	}

	return this->screen->logicalDotsPerInch() / 25.4;
}

qreal QuickshellScreenInfo::devicePixelRatio() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0.0;
	}

	return this->screen->devicePixelRatio();
}

Qt::ScreenOrientation QuickshellScreenInfo::orientation() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return Qt::PrimaryOrientation;
	}

	return this->screen->orientation();
}

Qt::ScreenOrientation QuickshellScreenInfo::primaryOrientation() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return Qt::PrimaryOrientation;
	}

	return this->screen->primaryOrientation();
}

void QuickshellScreenInfo::screenDestroyed() {
	this->screen = nullptr;
	this->dangling = true;
}

QDebug operator<<(QDebug debug, const QuickshellScreenInfo* screen) {
	if (screen == nullptr) {
		debug.nospace() << "QuickshellScreenInfo(nullptr)";
		return debug;
	}

	debug.nospace() << screen->metaObject()->className() << '(' << static_cast<const void*>(screen)
	                << ", screen=" << screen->screen << ')';

	return debug;
}

QString QuickshellScreenInfo::toString() const {
	QString str;
	QDebug(&str) << this;
	return str;
}
