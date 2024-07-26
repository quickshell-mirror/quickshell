#pragma once

#include <functional>

#include <qaction.h>
#include <qactiongroup.h>
#include <qcontainerfwd.h>
#include <qmenu.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "popupanchor.hpp"
#include "qsmenu.hpp"

namespace qs::menu::platform {

class PlatformMenuQMenu: public QMenu {
public:
	explicit PlatformMenuQMenu() = default;
	~PlatformMenuQMenu() override;
	Q_DISABLE_COPY_MOVE(PlatformMenuQMenu);

	void setVisible(bool visible) override;

	PlatformMenuQMenu* containingMenu = nullptr;
	QPoint targetPosition;
};

class PlatformMenuEntry: public QObject {
	Q_OBJECT;

public:
	explicit PlatformMenuEntry(QsMenuEntry* menu);
	~PlatformMenuEntry() override;
	Q_DISABLE_COPY_MOVE(PlatformMenuEntry);

	bool display(QObject* parentWindow, int relativeX, int relativeY);
	bool display(PopupAnchor* anchor);

	static void registerCreationHook(std::function<void(PlatformMenuQMenu*)> hook);

signals:
	void closed();
	void relayoutParent();

public slots:
	void relayout();

private slots:
	void onAboutToShow();
	void onAboutToHide();
	void onActionTriggered();
	void onChildDestroyed();
	void onEnabledChanged();
	void onTextChanged();
	void onIconChanged();
	void onButtonTypeChanged();
	void onCheckStateChanged();

private:
	void clearChildren();
	void addToQMenu(PlatformMenuQMenu* menu);

	QsMenuEntry* menu;
	PlatformMenuQMenu* qmenu = nullptr;
	QAction* qaction = nullptr;
	QActionGroup* qactiongroup = nullptr;
	QVector<PlatformMenuEntry*> childEntries;
};

} // namespace qs::menu::platform
