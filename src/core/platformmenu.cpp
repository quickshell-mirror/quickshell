#include "platformmenu.hpp"
#include <functional>
#include <utility>

#include <qaction.h>
#include <qactiongroup.h>
#include <qapplication.h>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qicon.h>
#include <qlogging.h>
#include <qmenu.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../window/proxywindow.hpp"
#include "../window/windowinterface.hpp"
#include "iconprovider.hpp"
#include "model.hpp"
#include "platformmenu_p.hpp"
#include "popupanchor.hpp"
#include "qsmenu.hpp"

namespace qs::menu::platform {

namespace {
QVector<std::function<void(PlatformMenuQMenu*)>> CREATION_HOOKS; // NOLINT
PlatformMenuQMenu* ACTIVE_MENU = nullptr;                        // NOLINT
} // namespace

PlatformMenuQMenu::~PlatformMenuQMenu() {
	if (this == ACTIVE_MENU) {
		ACTIVE_MENU = nullptr;
	}
}

void PlatformMenuQMenu::setVisible(bool visible) {
	if (visible) {
		for (auto& hook: CREATION_HOOKS) {
			hook(this);
		}
	} else {
		if (this == ACTIVE_MENU) {
			ACTIVE_MENU = nullptr;
		}
	}

	this->QMenu::setVisible(visible);
}

PlatformMenuEntry::PlatformMenuEntry(QsMenuEntry* menu): QObject(menu), menu(menu) {
	this->relayout();

	// clang-format off
	QObject::connect(menu, &QsMenuEntry::enabledChanged, this, &PlatformMenuEntry::onEnabledChanged);
	QObject::connect(menu, &QsMenuEntry::textChanged, this, &PlatformMenuEntry::onTextChanged);
	QObject::connect(menu, &QsMenuEntry::iconChanged, this, &PlatformMenuEntry::onIconChanged);
	QObject::connect(menu, &QsMenuEntry::buttonTypeChanged, this, &PlatformMenuEntry::onButtonTypeChanged);
	QObject::connect(menu, &QsMenuEntry::checkStateChanged, this, &PlatformMenuEntry::onCheckStateChanged);
	QObject::connect(menu, &QsMenuEntry::hasChildrenChanged, this, &PlatformMenuEntry::relayoutParent);
	QObject::connect(menu->children(), &UntypedObjectModel::valuesChanged, this, &PlatformMenuEntry::relayout);
	// clang-format on
}

PlatformMenuEntry::~PlatformMenuEntry() {
	this->clearChildren();
	delete this->qaction;
	delete this->qmenu;
}

void PlatformMenuEntry::registerCreationHook(std::function<void(PlatformMenuQMenu*)> hook) {
	CREATION_HOOKS.push_back(std::move(hook));
}

bool PlatformMenuEntry::display(QObject* parentWindow, int relativeX, int relativeY) {
	QWindow* window = nullptr;

	if (qobject_cast<QApplication*>(QCoreApplication::instance()) == nullptr) {
		qCritical() << "Cannot display PlatformMenuEntry as quickshell was not started in "
		               "QApplication mode.";
		qCritical() << "To use platform menus, add `//@ pragma UseQApplication` to the top of your "
		               "root QML file and restart quickshell.";
		return false;
	} else if (this->qmenu == nullptr) {
		qCritical() << "Cannot display PlatformMenuEntry as it is not a menu.";
		return false;
	} else if (parentWindow == nullptr) {
		qCritical() << "Cannot display PlatformMenuEntry with null parent window.";
		return false;
	} else if (auto* proxy = qobject_cast<ProxyWindowBase*>(parentWindow)) {
		window = proxy->backingWindow();
	} else if (auto* interface = qobject_cast<WindowInterface*>(parentWindow)) {
		window = interface->proxyWindow()->backingWindow();
	} else {
		qCritical() << "PlatformMenuEntry.display() must be called with a window.";
		return false;
	}

	if (window == nullptr) {
		qCritical() << "Cannot display PlatformMenuEntry from a parent window that is not visible.";
		return false;
	}

	if (ACTIVE_MENU && this->qmenu != ACTIVE_MENU) {
		ACTIVE_MENU->close();
	}

	ACTIVE_MENU = this->qmenu;

	auto point = window->mapToGlobal(QPoint(relativeX, relativeY));

	this->qmenu->createWinId();
	this->qmenu->windowHandle()->setTransientParent(window);

	// Skips screen edge repositioning so it can be left to the compositor on wayland.
	this->qmenu->targetPosition = point;
	this->qmenu->popup(point);

	return true;
}

bool PlatformMenuEntry::display(PopupAnchor* anchor) {
	if (qobject_cast<QApplication*>(QCoreApplication::instance()) == nullptr) {
		qCritical() << "Cannot display PlatformMenuEntry as quickshell was not started in "
		               "QApplication mode.";
		qCritical() << "To use platform menus, add `//@ pragma UseQApplication` to the top of your "
		               "root QML file and restart quickshell.";
		return false;
	} else if (!anchor->backingWindow() || !anchor->backingWindow()->isVisible()) {
		qCritical() << "Cannot display PlatformMenuEntry on anchor without visible window.";
		return false;
	}

	if (ACTIVE_MENU && this->qmenu != ACTIVE_MENU) {
		ACTIVE_MENU->close();
	}

	ACTIVE_MENU = this->qmenu;

	this->qmenu->createWinId();
	this->qmenu->windowHandle()->setTransientParent(anchor->backingWindow());

	// Update the window geometry to the menu's actual dimensions so reposition
	// can accurately adjust it if applicable for the current platform.
	this->qmenu->windowHandle()->setGeometry({{0, 0}, this->qmenu->sizeHint()});

	PopupPositioner::instance()->reposition(anchor, this->qmenu->windowHandle(), false);

	// Open the menu at the position determined by the popup positioner.
	this->qmenu->popup(this->qmenu->windowHandle()->position());

	return true;
}

void PlatformMenuEntry::relayout() {
	if (qobject_cast<QApplication*>(QCoreApplication::instance()) == nullptr) {
		return;
	}

	if (this->menu->hasChildren()) {
		delete this->qaction;
		this->qaction = nullptr;

		if (this->qmenu == nullptr) {
			this->qmenu = new PlatformMenuQMenu();
			QObject::connect(this->qmenu, &QMenu::aboutToShow, this, &PlatformMenuEntry::onAboutToShow);
			QObject::connect(this->qmenu, &QMenu::aboutToHide, this, &PlatformMenuEntry::onAboutToHide);
		} else {
			this->clearChildren();
		}

		this->qmenu->setTitle(this->menu->text());

		auto icon = this->menu->icon();
		if (!icon.isEmpty()) {
			this->qmenu->setIcon(getCurrentEngineImageAsIcon(icon));
		}

		const auto& children = this->menu->children()->valueList();
		auto len = children.count();
		for (auto i = 0; i < len; i++) {
			auto* child = children.at(i);

			auto* instance = new PlatformMenuEntry(child);
			QObject::connect(instance, &QObject::destroyed, this, &PlatformMenuEntry::onChildDestroyed);

			QObject::connect(
			    instance,
			    &PlatformMenuEntry::relayoutParent,
			    this,
			    &PlatformMenuEntry::relayout
			);

			this->childEntries.push_back(instance);
			instance->addToQMenu(this->qmenu);
		}
	} else if (!this->menu->isSeparator()) {
		this->clearChildren();
		delete this->qmenu;
		this->qmenu = nullptr;

		if (this->qaction == nullptr) {
			this->qaction = new QAction(this);

			QObject::connect(
			    this->qaction,
			    &QAction::triggered,
			    this,
			    &PlatformMenuEntry::onActionTriggered
			);
		}

		this->qaction->setText(this->menu->text());

		auto icon = this->menu->icon();
		if (!icon.isEmpty()) {
			this->qaction->setIcon(getCurrentEngineImageAsIcon(icon));
		}

		this->qaction->setEnabled(this->menu->enabled());
		this->qaction->setCheckable(this->menu->buttonType() != QsMenuButtonType::None);

		if (this->menu->buttonType() == QsMenuButtonType::RadioButton) {
			if (!this->qactiongroup) this->qactiongroup = new QActionGroup(this);
			this->qaction->setActionGroup(this->qactiongroup);
		}

		this->qaction->setChecked(this->menu->checkState() != Qt::Unchecked);
	} else {
		delete this->qmenu;
		delete this->qaction;
		this->qmenu = nullptr;
		this->qaction = nullptr;
	}
}

void PlatformMenuEntry::onAboutToShow() { this->menu->ref(); }

void PlatformMenuEntry::onAboutToHide() {
	this->menu->unref();
	emit this->closed();
}

void PlatformMenuEntry::onActionTriggered() {
	auto* action = qobject_cast<PlatformMenuEntry*>(this->sender()->parent());
	emit action->menu->triggered();
}

void PlatformMenuEntry::onChildDestroyed() { this->childEntries.removeOne(this->sender()); }

void PlatformMenuEntry::onEnabledChanged() {
	if (this->qaction != nullptr) {
		this->qaction->setEnabled(this->menu->enabled());
	}
}

void PlatformMenuEntry::onTextChanged() {
	if (this->qmenu != nullptr) {
		this->qmenu->setTitle(this->menu->text());
	} else if (this->qaction != nullptr) {
		this->qaction->setText(this->menu->text());
	}
}

void PlatformMenuEntry::onIconChanged() {
	if (this->qmenu == nullptr && this->qaction == nullptr) return;

	auto iconName = this->menu->icon();
	QIcon icon;

	if (!iconName.isEmpty()) {
		icon = getCurrentEngineImageAsIcon(iconName);
	}

	if (this->qmenu != nullptr) {
		this->qmenu->setIcon(icon);
	} else if (this->qaction != nullptr) {
		this->qaction->setIcon(icon);
	}
}

void PlatformMenuEntry::onButtonTypeChanged() {
	if (this->qaction != nullptr) {
		QActionGroup* group = nullptr;

		if (this->menu->buttonType() == QsMenuButtonType::RadioButton) {
			if (!this->qactiongroup) this->qactiongroup = new QActionGroup(this);
			group = this->qactiongroup;
		}

		this->qaction->setActionGroup(group);
	}
}

void PlatformMenuEntry::onCheckStateChanged() {
	if (this->qaction != nullptr) {
		this->qaction->setChecked(this->menu->checkState() != Qt::Unchecked);
	}
}

void PlatformMenuEntry::clearChildren() {
	for (auto* child: this->childEntries) {
		delete child;
	}

	this->childEntries.clear();
}

void PlatformMenuEntry::addToQMenu(PlatformMenuQMenu* menu) {
	if (this->qmenu != nullptr) {
		menu->addMenu(this->qmenu);
		this->qmenu->containingMenu = menu;
	} else if (this->qaction != nullptr) {
		menu->addAction(this->qaction);
	} else {
		menu->addSeparator();
	}
}

} // namespace qs::menu::platform
