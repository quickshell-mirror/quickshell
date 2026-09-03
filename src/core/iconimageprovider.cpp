#include "iconimageprovider.hpp"
#include <algorithm>

#include <qcolor.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qicon.h>
#include <qlogging.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qurl.h>
#include <qurlquery.h>

namespace {

struct IconRequest {
	QString iconName;
	QString fallbackName;
	QString themePath;
};

QSize normalizedRequestedSize(const QSize& requestedSize) {
	auto targetSize = requestedSize.isValid() ? requestedSize : QSize(100, 100);
	if (targetSize.width() == 0 || targetSize.height() == 0) targetSize = QSize(2, 2);
	return targetSize;
}

QString normalizedLocalFile(const QString& value) {
	if (value.isEmpty()) return QString();

	auto url = QUrl(value);
	if (url.isLocalFile()) return url.toLocalFile();

	return value;
}

QString iconNameWithoutExtension(const QString& iconName) {
	auto info = QFileInfo(iconName);
	auto suffix = info.suffix();
	if (suffix.isEmpty()) return iconName;

	static const auto iconSuffixes = QStringList {
	    "png",
	    "svg",
	    "svgz",
	    "xpm",
	    "ico",
	};

	if (!iconSuffixes.contains(suffix, Qt::CaseInsensitive)) return iconName;

	auto baseName = iconName;
	baseName.chop(suffix.length() + 1);
	return baseName;
}

QStringList iconNameCandidates(const QString& iconName) {
	auto name = normalizedLocalFile(iconName.trimmed());
	if (name.isEmpty()) return {};

	auto candidates = QStringList {name};
	auto withoutExtension = iconNameWithoutExtension(name);
	if (withoutExtension != name) candidates.push_back(withoutExtension);

	auto baseName = QFileInfo(name).fileName();
	if (!baseName.isEmpty() && baseName != name) {
		candidates.push_back(baseName);
		auto baseWithoutExtension = iconNameWithoutExtension(baseName);
		if (baseWithoutExtension != baseName) candidates.push_back(baseWithoutExtension);
	}

	candidates.removeDuplicates();
	return candidates;
}

QStringList iconThemeSearchPaths(const QString& rawPath) {
	auto path = normalizedLocalFile(rawPath.trimmed());
	if (path.isEmpty()) return {};

	auto info = QFileInfo(path);
	if (info.isFile()) path = info.absolutePath();

	auto paths = QStringList {path};
	auto dir = QFileInfo(path);
	for (auto i = 0; i < 8 && dir.exists(); ++i) {
		if (QFileInfo(dir.absoluteFilePath() + "/index.theme").exists()) {
			auto parent = dir.dir().absolutePath();
			if (!parent.isEmpty()) paths.push_front(parent);
			break;
		}

		auto parent = dir.dir();
		if (parent.absolutePath() == dir.absoluteFilePath()) break;
		dir = QFileInfo(parent.absolutePath());
	}

	paths.removeDuplicates();
	return paths;
}

class IconThemeSearchPathScope {
public:
	explicit IconThemeSearchPathScope(const QString& themePath)
	    : oldSearchPaths(QIcon::themeSearchPaths()) {
		auto searchPaths = iconThemeSearchPaths(themePath);
		if (searchPaths.isEmpty()) return;

		auto newSearchPaths = this->oldSearchPaths;
		for (const auto& path: searchPaths) {
			if (!newSearchPaths.contains(path)) newSearchPaths.push_back(path);
		}

		QIcon::setThemeSearchPaths(newSearchPaths);
		this->active = true;
	}

	~IconThemeSearchPathScope() {
		if (this->active) QIcon::setThemeSearchPaths(this->oldSearchPaths);
	}

private:
	QStringList oldSearchPaths;
	bool active = false;
};

QStringList directFileCandidates(const QString& iconName, const QString& themePath) {
	auto names = iconNameCandidates(iconName);
	auto themePathValue = normalizedLocalFile(themePath.trimmed());
	auto candidates = QStringList();

	for (const auto& name: names) {
		if (QFileInfo(name).isAbsolute()) candidates.push_back(name);
	}

	if (!themePathValue.isEmpty()) {
		auto themeInfo = QFileInfo(themePathValue);
		auto baseDir = themeInfo.isFile() ? themeInfo.absolutePath() : themePathValue;
		for (const auto& name: names) {
			candidates.push_back(QDir(baseDir).absoluteFilePath(name));
			candidates.push_back(QDir(baseDir).absoluteFilePath(QFileInfo(name).fileName()));
		}

		auto dir = QDir(baseDir);
		for (const auto& name: names) {
			auto exact = QFileInfo(name).fileName();
			auto stem = iconNameWithoutExtension(exact);
			auto entries = dir.entryInfoList(
			    QStringList {exact, stem + ".*"},
			    QDir::Files | QDir::Readable,
			    QDir::Name
			);
			for (const auto& entry: entries) {
				candidates.push_back(entry.absoluteFilePath());
			}
		}
	}

	candidates.removeDuplicates();
	return candidates;
}

QIcon iconFromFileCandidates(const QString& iconName, const QString& themePath) {
	for (const auto& file: directFileCandidates(iconName, themePath)) {
		if (!QFileInfo(file).isFile()) continue;
		auto icon = QIcon(file);
		if (!icon.isNull()) return icon;
	}

	return QIcon();
}

QPixmap pixmapFromThemedIcon(const QString& iconName, const QString& themePath, const QSize& size) {
	IconThemeSearchPathScope searchPathScope(themePath);
	return QIcon::fromTheme(iconName).pixmap(size.width(), size.height());
}

QPixmap
pixmapFromFileCandidates(const QString& iconName, const QString& themePath, const QSize& size) {
	auto icon = iconFromFileCandidates(iconName, themePath);
	if (icon.isNull()) return QPixmap();

	return icon.pixmap(size.width(), size.height());
}

IconRequest parseIconRequest(const QString& id) {
	auto request = IconRequest();
	auto splitIdx = id.indexOf('?');

	if (splitIdx == -1) {
		request.iconName = id;
		return request;
	}

	request.iconName = id.sliced(0, splitIdx);
	auto queryString = id.sliced(splitIdx + 1);

	// Older request strings used "?path=... ?fallback=..." instead of '&'.
	auto malformedFallbackIdx = queryString.indexOf("?fallback=");
	if (malformedFallbackIdx != -1) {
		auto fallback = queryString.sliced(malformedFallbackIdx + 10);
		queryString = queryString.sliced(0, malformedFallbackIdx) + "&fallback=" + fallback;
	}

	auto query = QUrlQuery(queryString);
	request.themePath = query.queryItemValue("path", QUrl::FullyDecoded);
	request.fallbackName = query.queryItemValue("fallback", QUrl::FullyDecoded);
	return request;
}

} // namespace

QPixmap
IconImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
	auto request = parseIconRequest(id);
	auto pixmap = IconImageProvider::requestPixmapForIcon(
	    request.iconName,
	    request.themePath,
	    request.fallbackName,
	    requestedSize
	);

	if (pixmap.isNull()) {
		auto targetSize = normalizedRequestedSize(requestedSize);
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

QPixmap IconImageProvider::requestPixmapForIcon(
    const QString& icon,
    const QString& path,
    const QString& fallback,
    const QSize& requestedSize
) {
	auto targetSize = normalizedRequestedSize(requestedSize);

	for (const auto& candidate: iconNameCandidates(icon)) {
		auto themedPixmap = pixmapFromThemedIcon(candidate, path, targetSize);
		if (!themedPixmap.isNull()) return themedPixmap;
	}

	for (const auto& candidate: iconNameCandidates(fallback)) {
		auto fallbackPixmap = pixmapFromThemedIcon(candidate, path, targetSize);
		if (!fallbackPixmap.isNull()) return fallbackPixmap;
	}

	auto filePixmap = pixmapFromFileCandidates(icon, path, targetSize);
	if (!filePixmap.isNull()) return filePixmap;

	return pixmapFromFileCandidates(fallback, path, targetSize);
}

QString IconImageProvider::requestString(
    const QString& icon,
    const QString& path,
    const QString& fallback
) {
	auto req = "image://icon/" + icon;
	auto query = QUrlQuery();

	if (!path.isEmpty()) {
		query.addQueryItem("path", path);
	}

	if (!fallback.isEmpty()) {
		query.addQueryItem("fallback", fallback);
	}

	auto queryString = query.toString(QUrl::FullyEncoded);
	if (!queryString.isEmpty()) {
		req += "?" + queryString;
	}

	return req;
}
