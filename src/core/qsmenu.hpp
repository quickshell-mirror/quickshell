#pragma once

#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "doc.hpp"
#include "model.hpp"

namespace qs::menu {

///! Button type associated with a QsMenuEntry.
/// See @@QsMenuEntry.buttonType.
class QsMenuButtonType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// This menu item does not have a checkbox or a radiobutton associated with it.
		None = 0,
		/// This menu item should draw a checkbox.
		CheckBox = 1,
		/// This menu item should draw a radiobutton.
		RadioButton = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(qs::menu::QsMenuButtonType::Enum value);
};

class QsMenuEntry;

///! Menu handle for QsMenuOpener
/// See @@QsMenuOpener.
class QsMenuHandle: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

public:
	explicit QsMenuHandle(QObject* parent): QObject(parent) {}

	virtual void refHandle() {}
	virtual void unrefHandle() {}

	[[nodiscard]] virtual QsMenuEntry* menu() = 0;

signals:
	void menuChanged();
};

class QsMenuEntry: public QsMenuHandle {
	Q_OBJECT;
	/// If this menu item should be rendered as a separator between other items.
	///
	/// No other properties have a meaningful value when @@isSeparator is true.
	Q_PROPERTY(bool isSeparator READ isSeparator NOTIFY isSeparatorChanged);
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged);
	/// Text of the menu item.
	Q_PROPERTY(QString text READ text NOTIFY textChanged);
	/// Url of the menu item's icon or `""` if it doesn't have one.
	///
	/// This can be passed to [Image.source](https://doc.qt.io/qt-6/qml-qtquick-image.html#source-prop)
	/// as shown below.
	///
	/// ```qml
	/// Image {
	///   source: menuItem.icon
	///   // To get the best image quality, set the image source size to the same size
	///   // as the rendered image.
	///   sourceSize.width: width
	///   sourceSize.height: height
	/// }
	/// ```
	Q_PROPERTY(QString icon READ icon NOTIFY iconChanged);
	/// If this menu item has an associated checkbox or radiobutton.
	Q_PROPERTY(qs::menu::QsMenuButtonType::Enum buttonType READ buttonType NOTIFY buttonTypeChanged);
	/// The check state of the checkbox or radiobutton if applicable, as a
	/// [Qt.CheckState](https://doc.qt.io/qt-6/qt.html#CheckState-enum).
	Q_PROPERTY(Qt::CheckState checkState READ checkState NOTIFY checkStateChanged);
	/// If this menu item has children that can be accessed through a @@QsMenuOpener$.
	Q_PROPERTY(bool hasChildren READ hasChildren NOTIFY hasChildrenChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("QsMenuEntry cannot be directly created");

public:
	explicit QsMenuEntry(QObject* parent): QsMenuHandle(parent) {}

	[[nodiscard]] QsMenuEntry* menu() override;

	/// Display a platform menu at the given location relative to the parent window.
	Q_INVOKABLE void display(QObject* parentWindow, qint32 relativeX, qint32 relativeY);

	[[nodiscard]] virtual bool isSeparator() const { return false; }
	[[nodiscard]] virtual bool enabled() const { return true; }
	[[nodiscard]] virtual QString text() const { return ""; }
	[[nodiscard]] virtual QString icon() const { return ""; }
	[[nodiscard]] virtual QsMenuButtonType::Enum buttonType() const { return QsMenuButtonType::None; }
	[[nodiscard]] virtual Qt::CheckState checkState() const { return Qt::Unchecked; }
	[[nodiscard]] virtual bool hasChildren() const { return false; }

	void ref();
	void unref();

	[[nodiscard]] virtual ObjectModel<QsMenuEntry>* children();

signals:
	/// Send a trigger/click signal to the menu entry.
	void triggered();

	QSDOC_HIDE void opened();
	QSDOC_HIDE void closed();

	void isSeparatorChanged();
	void enabledChanged();
	void textChanged();
	void iconChanged();
	void buttonTypeChanged();
	void checkStateChanged();
	void hasChildrenChanged();

private:
	qsizetype refcount = 0;
};

///! Provides access to children of a QsMenuEntry
class QsMenuOpener: public QObject {
	Q_OBJECT;
	/// The menu to retrieve children from.
	Q_PROPERTY(qs::menu::QsMenuHandle* menu READ menu WRITE setMenu NOTIFY menuChanged);
	/// The children of the given menu.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::menu::QsMenuEntry>*);
	Q_PROPERTY(UntypedObjectModel* children READ children NOTIFY childrenChanged);
	QML_ELEMENT;

public:
	explicit QsMenuOpener(QObject* parent = nullptr): QObject(parent) {}
	~QsMenuOpener() override;
	Q_DISABLE_COPY_MOVE(QsMenuOpener);

	[[nodiscard]] QsMenuHandle* menu() const;
	void setMenu(QsMenuHandle* menu);

	[[nodiscard]] ObjectModel<QsMenuEntry>* children();

signals:
	void menuChanged();
	void childrenChanged();

private slots:
	void onMenuDestroyed();

private:
	QsMenuHandle* mMenu = nullptr;
};

} // namespace qs::menu
