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

QsMenuEntry* QsMenuOpener::menu() const { return this->mMenu; }

void QsMenuOpener::setMenu(QsMenuEntry* menu) {
	if (menu == this->mMenu) return;

	if (this->mMenu != nullptr) {
		this->mMenu->unref();
		QObject::disconnect(this->mMenu, nullptr, this, nullptr);
	}

	this->mMenu = menu;

	if (menu != nullptr) {
		QObject::connect(menu, &QObject::destroyed, this, &QsMenuOpener::onMenuDestroyed);
		QObject::connect(menu, &QsMenuEntry::childrenChanged, this, &QsMenuOpener::childrenChanged);
		menu->ref();
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
	return this->mMenu ? this->mMenu->children() : QsMenuEntry::emptyChildren(this);
}

qsizetype QsMenuEntry::childCount(QQmlListProperty<QsMenuEntry>* /*property*/) { return 0; }

QsMenuEntry*
QsMenuEntry::childAt(QQmlListProperty<QsMenuEntry>* /*property*/, qsizetype /*index*/) {
	return nullptr;
}

} // namespace qs::menu
