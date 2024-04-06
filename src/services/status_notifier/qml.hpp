#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "item.hpp"

namespace SystemTrayStatus { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	// A passive item does not convey important information and can be considered idle. You may want to hide these.
	Passive = 0,
	// An active item may have information more important than a passive one and you probably do not want to hide it.
	Active = 1,
	// An item that needs attention conveys very important information such as low battery.
	NeedsAttention = 2,
};
Q_ENUM_NS(Enum);

} // namespace SystemTrayStatus

namespace SystemTrayCategory { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	// The fallback category for general applications or anything that does
	// not fit into a different category.
	ApplicationStatus = 0,
	// System services such as IMEs or disk indexing.
	SystemServices = 1,
	// Hardware controls like battery indicators or volume control.
	Hardware = 2,
};
Q_ENUM_NS(Enum);

} // namespace SystemTrayCategory

///! An item in the system tray.
/// A system tray item, roughly conforming to the [kde/freedesktop spec]
/// (there is no real spec, we just implemented whatever seemed to actually be used).
///
/// [kde/freedesktop spec]: https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem/
class SystemTrayItem: public QObject {
	Q_OBJECT;
	// A name unique to the application, such as its name.
	Q_PROPERTY(QString id READ id NOTIFY idChanged);
	// A name that describes the application
	Q_PROPERTY(QString title READ title NOTIFY titleChanged);
	Q_PROPERTY(SystemTrayStatus::Enum status READ status NOTIFY statusChanged);
	Q_PROPERTY(SystemTrayCategory::Enum category READ category NOTIFY categoryChanged);
	// Icon source string, usable as an Image source.
	Q_PROPERTY(QString icon READ icon NOTIFY iconChanged);
	Q_PROPERTY(QString tooltipTitle READ tooltipTitle NOTIFY tooltipTitleChanged);
	Q_PROPERTY(QString tooltipDescription READ tooltipDescription NOTIFY tooltipDescriptionChanged);
	// If this tray item only offers a menu and no activation action.
	Q_PROPERTY(bool onlyMenu READ onlyMenu NOTIFY onlyMenuChanged);
	QML_ELEMENT;

public:
	explicit SystemTrayItem(
	    qs::service::sni::StatusNotifierItem* item = nullptr,
	    QObject* parent = nullptr
	);

	// Primary activation action, generally triggered via a left click.
	//Q_INVOKABLE void activate();

	// Secondary activation action, generally triggered via a middle click.
	//Q_INVOKABLE void secondaryActivate();

	// Scroll action, such as changing volume on a mixer.
	//Q_INVOKABLE void scroll(qint32 delta, bool horizontal);

	[[nodiscard]] QString id() const;
	[[nodiscard]] QString title() const;
	[[nodiscard]] SystemTrayStatus::Enum status() const;
	[[nodiscard]] SystemTrayCategory::Enum category() const;
	[[nodiscard]] QString icon() const;
	[[nodiscard]] QString tooltipTitle() const;
	[[nodiscard]] QString tooltipDescription() const;
	[[nodiscard]] bool onlyMenu() const;

signals:
	void idChanged();
	void titleChanged();
	void statusChanged();
	void categoryChanged();
	void iconChanged();
	void tooltipTitleChanged();
	void tooltipDescriptionChanged();
	void onlyMenuChanged();

private:
	qs::service::sni::StatusNotifierItem* item = nullptr;

	friend class SystemTray;
};

class SystemTray: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<SystemTrayItem> items READ items NOTIFY itemsChanged);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit SystemTray(QObject* parent = nullptr);

	[[nodiscard]] QQmlListProperty<SystemTrayItem> items();

signals:
	void itemsChanged();

private slots:
	void onItemRegistered(qs::service::sni::StatusNotifierItem* item);
	void onItemUnregistered(qs::service::sni::StatusNotifierItem* item);

private:
	static qsizetype itemsCount(QQmlListProperty<SystemTrayItem>* property);
	static SystemTrayItem* itemAt(QQmlListProperty<SystemTrayItem>* property, qsizetype index);

	QList<SystemTrayItem*> mItems;
};
