#pragma once

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsvalue.h>
#include <qlist.h>
#include <qobjectdefs.h>
#include <qqmlintegration.h>
#include <qqmlparserstatus.h>
#include <qstringview.h>
#include <qtmetamacros.h>

#include "fileview.hpp"

namespace qs::io {

/// See @@JsonAdapter.
class JsonObject: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
};

///! FileView adapter for accessing JSON files.
/// JsonAdapter is a @@FileView adapter that exposes a JSON file as a set of QML
/// properties that can be read and written to.
///
/// Each property defined in a JsonAdapter corresponds to a key in the JSON file.
/// Supported property types are:
/// - Primitves (`int`, `bool`, `string`, `real`)
/// - Sub-object adapters (@@JsonObject$)
/// - JSON objects and arrays, as a `var` type
/// - Lists of any of the above (`list<string>` etc)
///
/// When the @@FileView$'s data is loaded, properties of a JsonAdapter or
/// sub-object adapter (@@JsonObject$) are updated if their values have changed.
///
/// When properties of a JsonAdapter or sub-object adapter are changed from QML,
/// @@FileView.adapterUpdated(s) is emitted, which may be used to save the file's new
/// state (see @@FileView.writeAdapter()$).
///
/// ### Example
/// ```qml
/// @@FileView {
///   path: "/path/to/file"
///
///   // when changes are made on disk, reload the file's content
///   watchChanges: true
///   onFileChanged: reload()
///
///   // when changes are made to properties in the adapter, save them
///   onAdapterUpdated: writeAdapter()
///
///   JsonAdapter {
///     property string myStringProperty: "default value"
///     onMyStringPropertyChanged: {
///       console.log("myStringProperty was changed via qml or on disk")
///     }
///
///     property list<string> stringList: [ "default", "value" ]
///
///     property JsonObject subObject: JsonObject {
///       property string subObjectProperty: "default value"
///       onSubObjectPropertyChanged: console.log("same as above")
///     }
///
///     // works the same way as subObject
///     property var inlineJson: { "a": "b" }
///   }
/// }
/// ```
///
/// The above snippet produces the JSON document below:
/// ```json
/// {
///    "myStringProperty": "default value",
///    "stringList": [
///      "default",
///      "value"
///    ],
///    "subObject": {
///      "subObjectProperty": "default value"
///    },
///    "inlineJson": {
///      "a": "b"
///    }
/// }
/// ```
class JsonAdapter
    : public FileViewAdapter
    , public QQmlParserStatus {
	Q_OBJECT;
	QML_ELEMENT;
	Q_INTERFACES(QQmlParserStatus);

public:
	void classBegin() override {}
	void componentComplete() override;

	void deserializeAdapter(const QByteArray& data) override;
	[[nodiscard]] QByteArray serializeAdapter() override;

private slots:
	void onPropertyChanged();

private:
	void connectNotifiers();
	void connectNotifiersRec(int notifySlot, QObject* obj, const QMetaObject* base);
	void deserializeRec(const QJsonObject& json, QObject* obj, const QMetaObject* base);
	[[nodiscard]] QJsonObject serializeRec(const QObject* obj, const QMetaObject* base) const;

	bool changesBlocked = false;
	QList<JsonObject*> createdObjects;
	QList<JsonObject*> oldCreatedObjects;
};

} // namespace qs::io
