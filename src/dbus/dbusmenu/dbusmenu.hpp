#pragma once

#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickimageprovider.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/imageprovider.hpp"
#include "../properties.hpp"
#include "dbus_menu_types.hpp"

Q_DECLARE_LOGGING_CATEGORY(logDbusMenu);

namespace ToggleButtonType { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	None = 0,
	CheckBox = 1,
	RadioButton = 2,
};
Q_ENUM_NS(Enum);

} // namespace ToggleButtonType

class DBusMenuInterface;

namespace qs::dbus::dbusmenu {

QDebug operator<<(QDebug debug, const ToggleButtonType::Enum& toggleType);

class DBusMenu;
class DBusMenuPngImage;

class DBusMenuItem: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(DBusMenu* menuHandle READ menuHandle CONSTANT);
	Q_PROPERTY(QString label READ label NOTIFY labelChanged);
	Q_PROPERTY(QString cleanLabel READ cleanLabel NOTIFY labelChanged);
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged);
	Q_PROPERTY(QString icon READ icon NOTIFY iconChanged);
	Q_PROPERTY(ToggleButtonType::Enum toggleType READ toggleType NOTIFY toggleTypeChanged);
	Q_PROPERTY(Qt::CheckState checkState READ checkState NOTIFY checkStateChanged);
	Q_PROPERTY(bool isSeparator READ isSeparator NOTIFY separatorChanged);
	Q_PROPERTY(bool hasChildren READ hasChildren NOTIFY hasChildrenChanged);
	Q_PROPERTY(bool showChildren READ isShowingChildren WRITE setShowChildren NOTIFY showingChildrenChanged);
	Q_PROPERTY(QQmlListProperty<DBusMenuItem> children READ children NOTIFY childrenChanged);
	// clang-format on
	QML_NAMED_ELEMENT(DBusMenu);
	QML_UNCREATABLE("DBusMenus can only be acquired from a DBusMenuHandle");

public:
	explicit DBusMenuItem(qint32 id, DBusMenu* menu, DBusMenuItem* parentMenu);

	Q_INVOKABLE void click();
	Q_INVOKABLE void hover() const;

	[[nodiscard]] DBusMenu* menuHandle() const;
	[[nodiscard]] QString label() const;
	[[nodiscard]] QString cleanLabel() const;
	[[nodiscard]] bool enabled() const;
	[[nodiscard]] QString icon() const;
	[[nodiscard]] ToggleButtonType::Enum toggleType() const;
	[[nodiscard]] Qt::CheckState checkState() const;
	[[nodiscard]] bool isSeparator() const;
	[[nodiscard]] bool hasChildren() const;

	[[nodiscard]] bool isShowingChildren() const;
	void setShowChildren(bool showChildren);

	[[nodiscard]] QQmlListProperty<DBusMenuItem> children();

	void updateProperties(const QVariantMap& properties, const QStringList& removed = {});
	void onChildrenUpdated();

	qint32 id = 0;
	QString mLabel;
	QVector<qint32> mChildren;
	bool mShowChildren = false;
	bool childrenLoaded = false;
	DBusMenu* menu = nullptr;

signals:
	void labelChanged();
	//void mnemonicChanged();
	void enabledChanged();
	void iconChanged();
	void separatorChanged();
	void toggleTypeChanged();
	void checkStateChanged();
	void hasChildrenChanged();
	void showingChildrenChanged();
	void childrenChanged();

private:
	QString mCleanLabel;
	//QChar mnemonic;
	bool mEnabled = true;
	bool visible = true;
	bool mSeparator = false;
	QString iconName;
	DBusMenuPngImage* image = nullptr;
	ToggleButtonType::Enum mToggleType = ToggleButtonType::None;
	Qt::CheckState mCheckState = Qt::Checked;
	bool displayChildren = false;
	QVector<qint32> enabledChildren;
	DBusMenuItem* parentMenu = nullptr;

	static qsizetype childrenCount(QQmlListProperty<DBusMenuItem>* property);
	static DBusMenuItem* childAt(QQmlListProperty<DBusMenuItem>* property, qsizetype index);
};

QDebug operator<<(QDebug debug, DBusMenuItem* item);

class DBusMenu: public QObject {
	Q_OBJECT;
	Q_PROPERTY(DBusMenuItem* menu READ menu CONSTANT);
	QML_NAMED_ELEMENT(DBusMenuHandle);
	QML_UNCREATABLE("Menu handles cannot be directly created");

public:
	explicit DBusMenu(const QString& service, const QString& path, QObject* parent = nullptr);

	dbus::DBusPropertyGroup properties;
	dbus::DBusProperty<quint32> version {this->properties, "Version"};
	dbus::DBusProperty<QString> textDirection {this->properties, "TextDirection"};
	dbus::DBusProperty<QString> status {this->properties, "Status"};
	dbus::DBusProperty<QStringList> iconThemePath {this->properties, "IconThemePath"};

	void prepareToShow(qint32 item, bool sendOpened);
	void updateLayout(qint32 parent, qint32 depth);
	void removeRecursive(qint32 id);
	void sendEvent(qint32 item, const QString& event);

	DBusMenuItem rootItem {0, this, nullptr};
	QHash<qint32, DBusMenuItem*> items {std::make_pair(0, &this->rootItem)};

	[[nodiscard]] DBusMenuItem* menu();

private slots:
	void onLayoutUpdated(quint32 revision, qint32 parent);
	void onItemPropertiesUpdated(
	    const DBusMenuItemPropertiesList& updatedProps,
	    const DBusMenuItemPropertyNamesList& removedProps
	);

private:
	void updateLayoutRecursive(const DBusMenuLayout& layout, DBusMenuItem* parent, qint32 depth);

	DBusMenuInterface* interface = nullptr;
};

QDebug operator<<(QDebug debug, DBusMenu* menu);

class DBusMenuPngImage: public QsImageHandle {
public:
	explicit DBusMenuPngImage(QByteArray data, DBusMenuItem* parent)
	    : QsImageHandle(QQuickImageProvider::Image, parent)
	    , data(std::move(data)) {}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

	QByteArray data;
};

} // namespace qs::dbus::dbusmenu
