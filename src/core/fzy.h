#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qstring.h>

namespace qs {

///! A fzy finder.
/// A fzy finder.
///
/// You can use this singleton to filter desktop entries like below.
///
/// ```qml
/// model: ScriptModel {
///   values: FzyFinder.filter(search.text, @@DesktopEntries.applications.values, "name");
/// }
/// ```
class FzyFinder : public QObject {
	Q_OBJECT;
	QML_SINGLETON;
	QML_ELEMENT;

public:
	explicit FzyFinder(QObject* parent = nullptr): QObject(parent) {}

	/// Filters the list haystacks that don't contain the needle.
	/// `needle` is the query to search with.
	/// `haystacks` is what will be searched.
	/// `name` is a property of each object in `haystacks` if `haystacks[n].name` is not a `string` then it will be treated as an empty string.
	/// The returned list is the objects that contain the query in fzy score order.
	Q_INVOKABLE [[nodiscard]] static QList<QObject*> filter(const QString& needle, const QList<QObject*>& haystacks, const QString& name);
};

}
