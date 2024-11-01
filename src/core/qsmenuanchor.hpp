#pragma once

#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "platformmenu.hpp"
#include "popupanchor.hpp"
#include "qsmenu.hpp"

namespace qs::menu {

///! Display anchor for platform menus.
class QsMenuAnchor: public QObject {
	Q_OBJECT;
	/// The menu's anchor / positioner relative to another window. The menu will not be
	/// shown until it has a valid anchor.
	///
	/// > [!INFO] *The following is subject to change and NOT a guarantee of future behavior.*
	/// >
	/// > A snapshot of the anchor at the time @@opened(s) is emitted will be
	/// > used to position the menu. Additional changes to the anchor after this point
	/// > will not affect the placement of the menu.
	///
	/// You can set properties of the anchor like so:
	/// ```qml
	/// QsMenuAnchor {
	///   anchor.window: parentwindow
	///   // or
	///   anchor {
	///     window: parentwindow
	///   }
	/// }
	/// ```
	Q_PROPERTY(PopupAnchor* anchor READ anchor CONSTANT);
	/// The menu that should be displayed on this anchor.
	///
	/// See also: @@Quickshell.Services.SystemTray.SystemTrayItem.menu.
	Q_PROPERTY(qs::menu::QsMenuHandle* menu READ menu WRITE setMenu NOTIFY menuChanged);
	/// If the menu is currently open and visible.
	///
	/// See also: @@open(), @@close().
	Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged);
	QML_ELEMENT;

public:
	explicit QsMenuAnchor(QObject* parent = nullptr): QObject(parent) {}
	~QsMenuAnchor() override;
	Q_DISABLE_COPY_MOVE(QsMenuAnchor);

	/// Open the given menu on this menu Requires that @@anchor is valid.
	Q_INVOKABLE void open();
	/// Close the open menu.
	Q_INVOKABLE void close();

	[[nodiscard]] PopupAnchor* anchor();

	[[nodiscard]] QsMenuHandle* menu() const;
	void setMenu(QsMenuHandle* menu);

	[[nodiscard]] bool isVisible() const;

signals:
	/// Sent when the menu is displayed onscreen which may be after @@visible
	/// becomes true.
	void opened();
	/// Sent when the menu is closed.
	void closed();

	void menuChanged();
	void visibleChanged();

private slots:
	void onMenuChanged();
	void onMenuDestroyed();

private:
	void onClosed();

	PopupAnchor mAnchor {this};
	QsMenuHandle* mMenu = nullptr;
	bool mOpen = false;
	platform::PlatformMenuEntry* platformMenu = nullptr;
};

} // namespace qs::menu
