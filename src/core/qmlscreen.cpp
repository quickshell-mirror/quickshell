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
