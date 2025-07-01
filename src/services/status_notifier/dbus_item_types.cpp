#include "dbus_item_types.hpp"

#include <qdbusargument.h>
#include <qdebug.h>
#include <qendian.h>
#include <qimage.h>
#include <qlogging.h>
#include <qmetatype.h>
#include <qsysinfo.h>
#include <qtypes.h>

bool DBusSniIconPixmap::operator==(const DBusSniIconPixmap& other) const {
	return this->width == other.width && this->height == other.height && this->data == other.data;
}

bool DBusSniTooltip::operator==(const DBusSniTooltip& other) const {
	return this->icon == other.icon && this->title == other.title
	    && this->description == other.description && this->iconPixmaps == other.iconPixmaps;
}

QImage DBusSniIconPixmap::createImage() const {
	// fix byte order if on a little endian machine
	if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
		auto* newbuf = new quint32[this->data.size()];
		const auto* oldbuf = reinterpret_cast<const quint32*>(this->data.constData());

		for (uint i = 0; i < this->data.size() / sizeof(quint32); ++i) {
			newbuf[i] = qFromBigEndian(oldbuf[i]); // NOLINT
		}

		return QImage(
		    reinterpret_cast<const uchar*>(newbuf),
		    this->width,
		    this->height,
		    QImage::Format_ARGB32,
		    [](void* ptr) { delete[] reinterpret_cast<quint32*>(ptr); },
		    newbuf
		);
	} else {
		return QImage(
		    reinterpret_cast<const uchar*>(this->data.constData()),
		    this->width,
		    this->height,
		    QImage::Format_ARGB32
		);
	}
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusSniIconPixmap& pixmap) {
	argument.beginStructure();
	argument >> pixmap.width;
	argument >> pixmap.height;
	argument >> pixmap.data;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusSniIconPixmap& pixmap) {
	argument.beginStructure();
	argument << pixmap.width;
	argument << pixmap.height;
	argument << pixmap.data;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusSniIconPixmapList& pixmaps) {
	argument.beginArray();
	pixmaps.clear();

	while (!argument.atEnd()) {
		pixmaps.append(qdbus_cast<DBusSniIconPixmap>(argument));
	}

	argument.endArray();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusSniIconPixmapList& pixmaps) {
	argument.beginArray(qMetaTypeId<DBusSniIconPixmap>());

	for (const auto& pixmap: pixmaps) {
		argument << pixmap;
	}

	argument.endArray();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusSniTooltip& tooltip) {
	argument.beginStructure();
	argument >> tooltip.icon;
	argument >> tooltip.iconPixmaps;
	argument >> tooltip.title;
	argument >> tooltip.description;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusSniTooltip& tooltip) {
	argument.beginStructure();
	argument << tooltip.icon;
	argument << tooltip.iconPixmaps;
	argument << tooltip.title;
	argument << tooltip.description;
	argument.endStructure();
	return argument;
}

QDebug operator<<(QDebug debug, const DBusSniIconPixmap& pixmap) {
	debug.nospace() << "DBusSniIconPixmap(width=" << pixmap.width << ", height=" << pixmap.height
	                << ")";

	return debug;
}

QDebug operator<<(QDebug debug, const DBusSniTooltip& tooltip) {
	debug.nospace() << "DBusSniTooltip(title=" << tooltip.title
	                << ", description=" << tooltip.description << ", icon=" << tooltip.icon
	                << ", iconPixmaps=" << tooltip.iconPixmaps << ")";

	return debug;
}
