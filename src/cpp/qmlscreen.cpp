#include "qmlscreen.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtypes.h>

bool QtShellScreenInfo::operator==(QtShellScreenInfo& other) const {
	return this->screen == other.screen;
}

void warnNull() { qWarning() << "attempted to use dangling screen object"; }

QString QtShellScreenInfo::name() const {
	if (this->screen == nullptr) {
		warnNull();
		return "{ DANGLING SCREEN POINTER }";
	}

	return this->screen->name();
}

qint32 QtShellScreenInfo::width() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0;
	}

	return this->screen->size().width();
}

qint32 QtShellScreenInfo::height() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0;
	}

	return this->screen->size().height();
}

qreal QtShellScreenInfo::pixelDensity() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0.0;
	}

	return this->screen->physicalDotsPerInch() / 25.4;
}

qreal QtShellScreenInfo::logicalPixelDensity() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0.0;
	}

	return this->screen->logicalDotsPerInch() / 25.4;
}

qreal QtShellScreenInfo::devicePixelRatio() const {
	if (this->screen == nullptr) {
		warnNull();
		return 0.0;
	}

	return this->screen->devicePixelRatio();
}

Qt::ScreenOrientation QtShellScreenInfo::orientation() const {
	if (this->screen == nullptr) {
		warnNull();
		return Qt::PrimaryOrientation;
	}

	return this->screen->orientation();
}

Qt::ScreenOrientation QtShellScreenInfo::primaryOrientation() const {
	if (this->screen == nullptr) {
		warnNull();
		return Qt::PrimaryOrientation;
	}

	return this->screen->primaryOrientation();
}
