#include "qsmenuanchor.hpp"

#include <qapplication.h>
#include <qcoreapplication.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "platformmenu.hpp"
#include "popupanchor.hpp"
#include "qsmenu.hpp"

using qs::menu::platform::PlatformMenuEntry;

namespace qs::menu {

QsMenuAnchor::~QsMenuAnchor() { this->onClosed(); }

void QsMenuAnchor::open() {
	if (qobject_cast<QApplication*>(QCoreApplication::instance()) == nullptr) {
		qCritical() << "Cannot call QsMenuAnchor.open() as quickshell was not started in "
		               "QApplication mode.";
		qCritical() << "To use platform menus, add `//@ pragma UseQApplication` to the top of your "
		               "root QML file and restart quickshell.";
		return;
	}

	if (this->mOpen) {
		qCritical() << "Cannot call QsMenuAnchor.open() as it is already open.";
		return;
	}

	if (!this->mMenu) {
		qCritical() << "Cannot open QsMenuAnchor with no menu attached.";
		return;
	}

	this->mOpen = true;

	if (this->mMenu->menu()) this->onMenuChanged();
	QObject::connect(this->mMenu, &QsMenuHandle::menuChanged, this, &QsMenuAnchor::onMenuChanged);
	this->mMenu->refHandle();

	emit this->visibleChanged();
}

void QsMenuAnchor::onMenuChanged() {
	// close menu if the path changes
	if (this->platformMenu || !this->mMenu->menu()) {
		this->onClosed();
		return;
	}

	this->platformMenu = new PlatformMenuEntry(this->mMenu->menu());
	QObject::connect(this->platformMenu, &PlatformMenuEntry::closed, this, &QsMenuAnchor::onClosed);

	auto success = this->platformMenu->display(&this->mAnchor);
	if (!success) this->onClosed();
	else emit this->opened();
}

void QsMenuAnchor::close() {
	if (!this->mOpen) {
		qCritical() << "Cannot close QsMenuAnchor as it isn't open.";
		return;
	}

	this->onClosed();
}

void QsMenuAnchor::onClosed() {
	if (!this->mOpen) return;

	this->mOpen = false;

	if (this->platformMenu) {
		this->platformMenu->deleteLater();
		this->platformMenu = nullptr;
	}

	if (this->mMenu) {
		QObject::disconnect(
		    this->mMenu,
		    &QsMenuHandle::menuChanged,
		    this,
		    &QsMenuAnchor::onMenuChanged
		);

		this->mMenu->unrefHandle();
	}

	emit this->closed();
	emit this->visibleChanged();
}

PopupAnchor* QsMenuAnchor::anchor() { return &this->mAnchor; }

QsMenuHandle* QsMenuAnchor::menu() const { return this->mMenu; }

void QsMenuAnchor::setMenu(QsMenuHandle* menu) {
	if (menu == this->mMenu) return;

	if (this->mMenu != nullptr) {
		if (this->platformMenu != nullptr) this->platformMenu->deleteLater();
		QObject::disconnect(this->mMenu, nullptr, this, nullptr);
	}

	this->mMenu = menu;

	if (menu != nullptr) {
		QObject::connect(menu, &QObject::destroyed, this, &QsMenuAnchor::onMenuDestroyed);
	}

	emit this->menuChanged();
}

bool QsMenuAnchor::isVisible() const { return this->mOpen; }

void QsMenuAnchor::onMenuDestroyed() {
	this->mMenu = nullptr;
	this->onClosed();
	emit this->menuChanged();
}

} // namespace qs::menu
