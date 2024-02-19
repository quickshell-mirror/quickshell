#include "qmlscreen.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qscreen.h>
#include <qtypes.h>

QuickShellScreenInfo::QuickShellScreenInfo(QObject* parent, QScreen* screen):
    QObject(parent), screen(screen) {

	if (this->screen != nullptr) {
		// clang-format off
		QObject::connect(this->screen, &QScreen::geometryChanged, this, &QuickShellScreenInfo::geometryChanged);
		QObject::connect(this->screen, &QScreen::physicalDotsPerInchChanged, this, &QuickShellScreenInfo::physicalPixelDensityChanged);
		QObject::connect(this->screen, &QScreen::logicalDotsPerInchChanged, this, &QuickShellScreenInfo::logicalPixelDensityChanged);
		QObject::connect(this->screen, &QScreen::orientationChanged, this, &QuickShellScreenInfo::orientationChanged);
		QObject::connect(this->screen, &QScreen::primaryOrientationChanged, this, &QuickShellScreenInfo::primaryOrientationChanged);
		QObject::connect(this->screen, &QObject::destroyed, this, &QuickShellScreenInfo::screenDestroyed);
		// clang-format on
	}
}

bool QuickShellScreenInfo::operator==(QuickShellScreenInfo& other) const {
	return this->screen == other.screen;
}

void QuickShellScreenInfo::warnDangling() const {
	if (this->dangling) {
		qWarning() << "attempted to use dangling screen object";
	}
}

QString QuickShellScreenInfo::name() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return "{ NULL SCREEN }";
	}

	return this->screen->name();
}

qint32 QuickShellScreenInfo::width() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0;
	}

	return this->screen->size().width();
}

qint32 QuickShellScreenInfo::height() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0;
	}

	return this->screen->size().height();
}

qreal QuickShellScreenInfo::physicalPixelDensity() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0.0;
	}

	return this->screen->physicalDotsPerInch() / 25.4;
}

qreal QuickShellScreenInfo::logicalPixelDensity() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0.0;
	}

	return this->screen->logicalDotsPerInch() / 25.4;
}

qreal QuickShellScreenInfo::devicePixelRatio() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return 0.0;
	}

	return this->screen->devicePixelRatio();
}

Qt::ScreenOrientation QuickShellScreenInfo::orientation() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return Qt::PrimaryOrientation;
	}

	return this->screen->orientation();
}

Qt::ScreenOrientation QuickShellScreenInfo::primaryOrientation() const {
	if (this->screen == nullptr) {
		this->warnDangling();
		return Qt::PrimaryOrientation;
	}

	return this->screen->primaryOrientation();
}

void QuickShellScreenInfo::screenDestroyed() {
	this->screen = nullptr;
	this->dangling = true;
}
