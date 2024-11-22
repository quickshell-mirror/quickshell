#pragma once

#include <qdbusargument.h>
#include <qdebug.h>
#include <qlist.h>

struct DBusSniIconPixmap {
	qint32 width = 0;
	qint32 height = 0;
	QByteArray data;

	// valid only for the lifetime of the pixmap
	[[nodiscard]] QImage createImage() const;

	bool operator==(const DBusSniIconPixmap& other) const;
};

using DBusSniIconPixmapList = QList<DBusSniIconPixmap>;

struct DBusSniTooltip {
	QString icon;
	DBusSniIconPixmapList iconPixmaps;
	QString title;
	QString description;

	bool operator==(const DBusSniTooltip& other) const;
};

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusSniIconPixmap& pixmap);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusSniIconPixmap& pixmap);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusSniIconPixmapList& pixmaps);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusSniIconPixmapList& pixmaps);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusSniTooltip& tooltip);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusSniTooltip& tooltip);

QDebug operator<<(QDebug debug, const DBusSniIconPixmap& pixmap);
QDebug operator<<(QDebug debug, const DBusSniTooltip& tooltip);
