#include "iconimageprovider.hpp"
#include <algorithm>

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qdir.h>
#include <qfile.h>
#include <qicon.h>
#include <qlogging.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsettings.h>
#include <qsize.h>
#include <qstring.h>

namespace {
// Get the system's color scheme preference
bool getSystemDarkPreference() {
	// Check KDE
	const QSettings kdeSettings(QDir::homePath() + "/.config/kdeglobals", QSettings::IniFormat);
	if (kdeSettings.value("General/ColorScheme").toString().toLower().contains("dark")) {
		return true;
	}

	// Check GTK
	const QSettings gtk4Settings(
	    QDir::homePath() + "/.config/gtk-4.0/settings.ini",
	    QSettings::IniFormat
	);

	return gtk4Settings.value("Settings/gtk-application-prefer-dark-theme", false).toBool();
}

QStringList listThemes() {
	QStringList themes;
	for (const auto& basePath: QIcon::themeSearchPaths()) {
		const QDir dir(basePath);
		if (!dir.exists()) continue;
		themes.append(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
	}

	themes.removeDuplicates();
	themes.sort();

	return themes;
}

QStringList getThemes() {
	QStringList themes;

	const QStringList available = listThemes();
	const bool preferDark = getSystemDarkPreference();

	// If dark mode is preferred, prioritize dark themes
	if (preferDark) {
		for (const auto& theme: available) {
			const QString lower = theme.toLower();
			if (lower.contains("dark") || lower.contains("-night") || lower.contains("_dark")) {
				themes.append(theme);
			}
		}
	} else {
		for (const auto& theme: available) {
			const QString lower = theme.toLower();
			if (lower.contains("light") || lower.contains("-day") || lower.contains("_light")) {
				themes.append(theme);
			}
		}
	}

	for (const auto& theme: available) {
		if (!themes.contains(theme)) {
			themes.append(theme);
		}
	}

	return themes;
}

QIcon findIconAcrossThemes(const QString& iconName) {
	const QStringList themes = getThemes();
	const QStringList categories = {"status", "apps", "actions", "places", "devices"};
	const QStringList sizes = {"scalable", "symbolic", "16", "22", "24", "32", "48", "64", "128"};
	const QStringList extensions = {".png", ".svg"};

	for (const auto& base: QIcon::themeSearchPaths()) {
		for (const auto& theme: themes) {
			for (const auto& category: categories) {
				for (const auto& size: sizes) {
					for (const auto& ext: extensions) {
						const QString path =
						    QString("%1/%2/%3/%4/%5%6").arg(base, theme, category, size, iconName, ext);

						if (QFile::exists(path)) {
							return QIcon(path);
						}
					}
				}
			}
		}
	}

	return QIcon();
}

} // namespace

QPixmap
IconImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
	QString iconName;
	QString fallbackName;
	QString path;

	static bool pathsFixed = false;
	static bool themeSet = false;

	if (!pathsFixed) {
		auto paths = QIcon::themeSearchPaths();
		paths.append("/usr/local/share/icons"); // unix
		paths.append("/usr/share/icons");       // tux

		QIcon::setThemeSearchPaths(paths);
		pathsFixed = true;
	}

	if (!themeSet) {
		const QString selectedTheme = getThemes().first();
		if (!selectedTheme.isEmpty()) {
			QIcon::setThemeName(selectedTheme);
		}

		themeSet = true;
	}

	auto splitIdx = id.indexOf("?path=");
	if (splitIdx != -1) {
		iconName = id.sliced(0, splitIdx);
		path = id.sliced(splitIdx + 6);
		path = QString("/%1/%2").arg(path, iconName.sliced(iconName.lastIndexOf('/') + 1));
	} else {
		splitIdx = id.indexOf("?fallback=");
		if (splitIdx != -1) {
			iconName = id.sliced(0, splitIdx);
			fallbackName = id.sliced(splitIdx + 10);
		} else {
			iconName = id;
		}
	}

	auto icon = QIcon::fromTheme(iconName);

	if (icon.isNull() && !fallbackName.isEmpty()) icon = QIcon::fromTheme(fallbackName);
	if (icon.isNull()) {
		const QStringList loosePaths = {
		    // homedir
		    QString("%1/.local/share/icons/%2.png").arg(QDir::homePath(), iconName),
		    QString("%1/.local/share/icons/%2.svg").arg(QDir::homePath(), iconName),
		    // unix
		    QString("/usr/local/share/icons/%1.png").arg(iconName),
		    QString("/usr/local/share/icons/%1.svg").arg(iconName),
		    QString("/usr/local/share/pixmaps/%1.png").arg(iconName),
		    QString("/usr/local/share/pixmaps/%1.svg").arg(iconName),
		    // tux
		    QString("/usr/share/icons/%1.png").arg(iconName),
		    QString("/usr/share/icons/%1.svg").arg(iconName),
		    QString("/usr/share/pixmaps/%1.png").arg(iconName),
		    QString("/usr/share/pixmaps/%1.svg").arg(iconName),
		};

		for (const auto& loosePath: loosePaths) {
			if (QFile::exists(loosePath)) {
				icon = QIcon(loosePath);
				break;
			}
		}
	}

	if (icon.isNull() && !path.isEmpty()) icon = QPixmap(path);
	if (icon.isNull()) {
		icon = findIconAcrossThemes(iconName);
	}

	auto targetSize = requestedSize.isValid() ? requestedSize : QSize(100, 100);
	if (targetSize.width() == 0 || targetSize.height() == 0) targetSize = QSize(2, 2);
	auto pixmap = icon.pixmap(targetSize.width(), targetSize.height());

	if (pixmap.isNull()) {
		qWarning() << "Could not load icon" << id << "at size" << targetSize << "from request";
		pixmap = IconImageProvider::missingPixmap(targetSize);
	}

	if (size != nullptr) *size = pixmap.size();
	return pixmap;
}

QPixmap IconImageProvider::missingPixmap(const QSize& size) {
	auto width = size.width() % 2 == 0 ? size.width() : size.width() + 1;
	auto height = size.height() % 2 == 0 ? size.height() : size.height() + 1;
	width = std::max(width, 2);
	height = std::max(height, 2);

	auto pixmap = QPixmap(width, height);
	pixmap.fill(QColorConstants::Black);
	auto painter = QPainter(&pixmap);

	auto halfWidth = width / 2;
	auto halfHeight = height / 2;
	auto purple = QColor(0xd900d8);
	painter.fillRect(halfWidth, 0, halfWidth, halfHeight, purple);
	painter.fillRect(0, halfHeight, halfWidth, halfHeight, purple);
	return pixmap;
}

QString IconImageProvider::requestString(
    const QString& icon,
    const QString& path,
    const QString& fallback
) {
	auto req = "image://icon/" + icon;

	if (!path.isEmpty()) {
		req += "?path=" + path;
	}

	if (!fallback.isEmpty()) {
		req += "?fallback=" + fallback;
	}

	return req;
}
