#include "qsmenu.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "platformmenu.hpp"

using namespace qs::menu::platform;

namespace qs::menu {

QString QsMenuButtonType::toString(QsMenuButtonType::Enum value) {
	switch (value) {
	case QsMenuButtonType::None: return "None";
	case QsMenuButtonType::CheckBox: return "CheckBox";
	case QsMenuButtonType::RadioButton: return "RadioButton";
	default: return "Invalid button type";
	}
}

QsMenuEntry* QsMenuEntry::menu() { return this; }

void QsMenuEntry::display(QObject* parentWindow, int relativeX, int relativeY) {
	auto* platform = new PlatformMenuEntry(this);

	QObject::connect(platform, &PlatformMenuEntry::closed, platform, [=]() {
		platform->deleteLater();
	});

	auto success = platform->display(parentWindow, relativeX, relativeY);
	if (!success) delete platform;
}

QQmlListProperty<QsMenuEntry> QsMenuEntry::emptyChildren(QObject* parent) {
	return QQmlListProperty<QsMenuEntry>(
	    parent,
	    nullptr,
	    &QsMenuEntry::childCount,
	    &QsMenuEntry::childAt
	);
}

void QsMenuEntry::ref() {
	this->refcount++;
	if (this->refcount == 1) emit this->opened();
}

void QsMenuEntry::unref() {
	this->refcount--;
	if (this->refcount == 0) emit this->closed();
}

QQmlListProperty<QsMenuEntry> QsMenuEntry::children() { return QsMenuEntry::emptyChildren(this); }

QsMenuOpener::~QsMenuOpener() {
	if (this->mMenu) {
		if (this->mMenu->menu()) this->mMenu->menu()->unref();
		this->mMenu->unrefHandle();
	}
}

QsMenuHandle* QsMenuOpener::menu() const { return this->mMenu; }

void QsMenuOpener::setMenu(QsMenuHandle* menu) {
	if (menu == this->mMenu) return;

	if (this->mMenu != nullptr) {
		QObject::disconnect(this->mMenu, nullptr, this, nullptr);

		if (this->mMenu->menu()) {
			QObject::disconnect(this->mMenu->menu(), nullptr, this, nullptr);
			this->mMenu->menu()->unref();
		}

		this->mMenu->unrefHandle();
	}

	this->mMenu = menu;

	if (menu != nullptr) {
		auto onMenuChanged = [this, menu]() {
			if (menu->menu()) {
				QObject::connect(
				    menu->menu(),
				    &QsMenuEntry::childrenChanged,
				    this,
				    &QsMenuOpener::childrenChanged
				);

				menu->menu()->ref();
			}

			emit this->childrenChanged();
		};

		QObject::connect(menu, &QObject::destroyed, this, &QsMenuOpener::onMenuDestroyed);
		QObject::connect(menu, &QsMenuHandle::menuChanged, this, onMenuChanged);

		if (menu->menu()) onMenuChanged();
		menu->refHandle();
	}

	emit this->menuChanged();
	emit this->childrenChanged();
}

void QsMenuOpener::onMenuDestroyed() {
	this->mMenu = nullptr;
	emit this->menuChanged();
	emit this->childrenChanged();
}

QQmlListProperty<QsMenuEntry> QsMenuOpener::children() {
	if (this->mMenu && this->mMenu->menu()) {
		return this->mMenu->menu()->children();
	} else {
		return QsMenuEntry::emptyChildren(this);
	}
}

qsizetype QsMenuEntry::childCount(QQmlListProperty<QsMenuEntry>* /*property*/) { return 0; }

QsMenuEntry*
QsMenuEntry::childAt(QQmlListProperty<QsMenuEntry>* /*property*/, qsizetype /*index*/) {
	return nullptr;
}

} // namespace qs::menu
