#include "qmlscreen.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtypes.h>

bool QuickShellScreenInfo::operator==(QuickShellScreenInfo& other) const {
	return this->screen == other.screen;
}

void warnNull() { qWarning() << "attempted to use dangling screen object"; }

QString QuickShellScreenInfo::name() const {
	if (this->screen == nullptr) {
		warnNull();
		return "{ DANGLING SCREEN POINTER }";
	}

	return this->screen->name();
}

qint32 QuickShellScreenInfo::width() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0;
	}

	return this->screen->size().width();
}

qint32 QuickShellScreenInfo::height() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0;
	}

	return this->screen->size().height();
}

qreal QuickShellScreenInfo::pixelDensity() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0.0;
	}

	return this->screen->physicalDotsPerInch() / 25.4;
}

qreal QuickShellScreenInfo::logicalPixelDensity() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0.0;
	}

	return this->screen->logicalDotsPerInch() / 25.4;
}

qreal QuickShellScreenInfo::devicePixelRatio() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0.0;
	}

	return this->screen->devicePixelRatio();
}

Qt::ScreenOrientation QuickShellScreenInfo::orientation() const {
	if (this->screen == nullptr) {
		warnNull();
		return Qt::PrimaryOrientation;
	}

	return this->screen->orientation();
}

Qt::ScreenOrientation QuickShellScreenInfo::primaryOrientation() const {
	if (this->screen == nullptr) {
		warnNull();
		return Qt::PrimaryOrientation;
	}

	return this->screen->primaryOrientation();
}
