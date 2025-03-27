#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../../core/doc.hpp"
#include "../../core/model.hpp"
#include "item.hpp"

///! System tray
/// Referencing the SystemTray singleton will make quickshell start tracking
/// system tray contents, which are updated as the tray changes, and can be
/// accessed via the @@items property.
class SystemTray: public QObject {
	Q_OBJECT;
	/// List of all system tray icons.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::sni::StatusNotifierItem>*);
	Q_PROPERTY(UntypedObjectModel* items READ items CONSTANT);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit SystemTray(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<qs::service::sni::StatusNotifierItem>* items();

private slots:
	void onItemRegistered(qs::service::sni::StatusNotifierItem* item);
	void onItemUnregistered(qs::service::sni::StatusNotifierItem* item);

private:
	static bool
	compareItems(qs::service::sni::StatusNotifierItem* a, qs::service::sni::StatusNotifierItem* b);

	ObjectModel<qs::service::sni::StatusNotifierItem> mItems {this};
};
