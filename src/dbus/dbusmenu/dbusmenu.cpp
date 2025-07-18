#include "dbusmenu.hpp"
#include <algorithm>

#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdebug.h>
#include <qimage.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../core/iconimageprovider.hpp"
#include "../../core/logcat.hpp"
#include "../../core/model.hpp"
#include "../../core/qsmenu.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_menu.h"
#include "dbus_menu_types.hpp"

QS_LOGGING_CATEGORY(logDbusMenu, "quickshell.dbus.dbusmenu", QtWarningMsg);

using namespace qs::menu;

namespace qs::dbus::dbusmenu {

DBusMenuItem::DBusMenuItem(qint32 id, DBusMenu* menu, DBusMenuItem* parentMenu)
    : QsMenuEntry(menu)
    , id(id)
    , menu(menu)
    , parentMenu(parentMenu) {
	QObject::connect(this, &QsMenuEntry::opened, this, &DBusMenuItem::sendOpened);
	QObject::connect(this, &QsMenuEntry::closed, this, &DBusMenuItem::sendClosed);
	QObject::connect(this, &QsMenuEntry::triggered, this, &DBusMenuItem::sendTriggered);

	QObject::connect(this->menu, &DBusMenu::iconThemePathChanged, this, &DBusMenuItem::iconChanged);
}

void DBusMenuItem::sendOpened() const { this->menu->sendEvent(this->id, "opened"); }
void DBusMenuItem::sendClosed() const { this->menu->sendEvent(this->id, "closed"); }
void DBusMenuItem::sendTriggered() const { this->menu->sendEvent(this->id, "clicked"); }

DBusMenu* DBusMenuItem::menuHandle() const { return this->menu; }
bool DBusMenuItem::enabled() const { return this->mEnabled; }
QString DBusMenuItem::text() const { return this->mCleanLabel; }

QString DBusMenuItem::icon() const {
	if (!this->iconName.isEmpty()) {
		return IconImageProvider::requestString(
		    this->iconName,
		    this->menu->iconThemePath.value().join(':')
		);
	} else if (this->image.hasData()) {
		return this->image.url();
	} else return nullptr;
}

QsMenuButtonType::Enum DBusMenuItem::buttonType() const { return this->mButtonType; };
Qt::CheckState DBusMenuItem::checkState() const { return this->mCheckState; }
bool DBusMenuItem::isSeparator() const { return this->mSeparator; }

bool DBusMenuItem::isShowingChildren() const { return this->mShowChildren && this->childrenLoaded; }

void DBusMenuItem::setShowChildrenRecursive(bool showChildren) {
	if (showChildren == this->mShowChildren) return;
	this->mShowChildren = showChildren;
	this->childrenLoaded = false;

	if (showChildren) {
		this->menu->prepareToShow(this->id, -1);
	} else {
		if (!this->mChildren.isEmpty()) {
			for (auto child: this->mChildren) {
				this->menu->removeRecursive(child);
			}

			this->mChildren.clear();
			this->onChildrenUpdated();
		}
	}
}

void DBusMenuItem::updateLayout() const {
	if (!this->isShowingChildren()) return;
	this->menu->updateLayout(this->id, -1);
}

bool DBusMenuItem::hasChildren() const { return this->displayChildren || this->id == 0; }

ObjectModel<QsMenuEntry>* DBusMenuItem::children() {
	return reinterpret_cast<ObjectModel<QsMenuEntry>*>(&this->enabledChildren);
}

void DBusMenuItem::updateProperties(const QVariantMap& properties, const QStringList& removed) {
	// Some programs appear to think sending an empty map does not mean "reset everything"
	// and instead means "do nothing". oh well...
	if (properties.isEmpty() && removed.isEmpty()) {
		qCDebug(logDbusMenu) << "Ignoring empty property update for" << this;
		return;
	}

	auto originalText = this->mText;
	//auto originalMnemonic = this->mnemonic;
	auto originalEnabled = this->mEnabled;
	auto originalVisible = this->visible;
	auto originalIconName = this->iconName;
	auto imageChanged = false;
	auto originalIsSeparator = this->mSeparator;
	auto originalButtonType = this->mButtonType;
	auto originalToggleState = this->mCheckState;
	auto originalDisplayChildren = this->displayChildren;

	auto label = properties.value("label");
	if (label.canConvert<QString>()) {
		auto text = label.value<QString>();
		this->mText = text;
		this->mCleanLabel = text;
		//this->mnemonic = QChar();

		for (auto i = 0; i < this->mText.length() - 1;) {
			if (this->mText.at(i) == '_') {
				//if (this->mnemonic == QChar()) this->mnemonic = this->mLabel.at(i + 1);
				this->mText.remove(i, 1);
				this->mText.insert(i + 1, "</u>");
				this->mText.insert(i, "<u>");
				i += 8;
			} else {
				i++;
			}
		}

		for (auto i = 0; i < this->mCleanLabel.length() - 1; i++) {
			if (this->mCleanLabel.at(i) == '_') {
				this->mCleanLabel.remove(i, 1);
			}
		}
	} else if (removed.isEmpty() || removed.contains("label")) {
		this->mText = "";
		//this->mnemonic = QChar();
	}

	auto enabled = properties.value("enabled");
	if (enabled.canConvert<bool>()) {
		this->mEnabled = enabled.value<bool>();
	} else if (removed.isEmpty() || removed.contains("enabled")) {
		this->mEnabled = true;
	}

	auto visible = properties.value("visible");
	if (visible.canConvert<bool>()) {
		this->visible = visible.value<bool>();
	} else if (removed.isEmpty() || removed.contains("visible")) {
		this->visible = true;
	}

	auto iconName = properties.value("icon-name");
	if (iconName.canConvert<QString>()) {
		this->iconName = iconName.value<QString>();
	} else if (removed.isEmpty() || removed.contains("icon-name")) {
		this->iconName = "";
	}

	auto iconData = properties.value("icon-data");
	if (iconData.canConvert<QByteArray>()) {
		auto data = iconData.value<QByteArray>();
		if (data.isEmpty()) {
			imageChanged = this->image.hasData();
			this->image.data.clear();
		} else if (!this->image.hasData() || this->image.data != data) {
			imageChanged = true;
			this->image.data = data;
			this->image.imageChanged();
		}
	} else if (removed.isEmpty() || removed.contains("icon-data")) {
		imageChanged = this->image.hasData();
		image.data.clear();
	}

	auto type = properties.value("type");
	if (type.canConvert<QString>()) {
		this->mSeparator = type.value<QString>() == "separator";
	} else if (removed.isEmpty() || removed.contains("type")) {
		this->mSeparator = false;
	}

	auto toggleType = properties.value("toggle-type");
	if (toggleType.canConvert<QString>()) {
		auto toggleTypeStr = toggleType.value<QString>();

		if (toggleTypeStr == "") this->mButtonType = QsMenuButtonType::None;
		else if (toggleTypeStr == "checkmark") this->mButtonType = QsMenuButtonType::CheckBox;
		else if (toggleTypeStr == "radio") this->mButtonType = QsMenuButtonType::RadioButton;
		else {
			qCWarning(logDbusMenu) << "Unrecognized toggle type" << toggleTypeStr << "for" << this;
			this->mButtonType = QsMenuButtonType::None;
		}
	} else if (removed.isEmpty() || removed.contains("toggle-type")) {
		this->mButtonType = QsMenuButtonType::None;
	}

	auto toggleState = properties.value("toggle-state");
	if (toggleState.canConvert<qint32>()) {
		auto toggleStateInt = toggleState.value<qint32>();

		if (toggleStateInt == 0) this->mCheckState = Qt::Unchecked;
		else if (toggleStateInt == 1) this->mCheckState = Qt::Checked;
		else this->mCheckState = Qt::PartiallyChecked;
	} else if (removed.isEmpty() || removed.contains("toggle-state")) {
		this->mCheckState = Qt::Unchecked;
	}

	auto childrenDisplay = properties.value("children-display");
	if (childrenDisplay.canConvert<QString>()) {
		auto childrenDisplayStr = childrenDisplay.value<QString>();

		if (childrenDisplayStr == "") this->displayChildren = false;
		else if (childrenDisplayStr == "submenu") this->displayChildren = true;
		else {
			qCWarning(logDbusMenu) << "Unrecognized children-display mode" << childrenDisplayStr << "for"
			                       << this;
			this->displayChildren = false;
		}
	} else if (removed.isEmpty() || removed.contains("children-display")) {
		this->displayChildren = false;
	}

	if (this->mText != originalText) emit this->textChanged();
	//if (this->mnemonic != originalMnemonic) emit this->labelChanged();
	if (this->mEnabled != originalEnabled) emit this->enabledChanged();
	if (this->visible != originalVisible && this->parentMenu != nullptr)
		this->parentMenu->onChildrenUpdated();
	if (this->mButtonType != originalButtonType) emit this->buttonTypeChanged();
	if (this->mCheckState != originalToggleState) emit this->checkStateChanged();
	if (this->mSeparator != originalIsSeparator) emit this->isSeparatorChanged();
	if (this->displayChildren != originalDisplayChildren) emit this->hasChildrenChanged();

	if (this->iconName != originalIconName || imageChanged) {
		emit this->iconChanged();
	}

	qCDebug(logDbusMenu).nospace() << "Updated properties of " << this << " { label=" << this->mText
	                               << ", enabled=" << this->mEnabled << ", visible=" << this->visible
	                               << ", iconName=" << this->iconName << ", iconData=" << &this->image
	                               << ", separator=" << this->mSeparator
	                               << ", toggleType=" << this->mButtonType
	                               << ", toggleState=" << this->mCheckState
	                               << ", displayChildren=" << this->displayChildren << " }";
}

void DBusMenuItem::onChildrenUpdated() {
	QVector<DBusMenuItem*> children;
	for (auto child: this->mChildren) {
		auto* item = this->menu->items.value(child);
		if (item->visible) children.append(item);
	}

	this->enabledChildren.diffUpdate(children);
}

QDebug operator<<(QDebug debug, DBusMenuItem* item) {
	if (item == nullptr) {
		debug << "DBusMenuItem(nullptr)";
		return debug;
	}

	auto saver = QDebugStateSaver(debug);
	debug.nospace() << "DBusMenuItem(" << static_cast<void*>(item) << ", id=" << item->id
	                << ", label=" << item->mText << ", menu=" << item->menu << ")";
	return debug;
}

DBusMenu::DBusMenu(const QString& service, const QString& path, QObject* parent): QObject(parent) {
	qDBusRegisterMetaType<DBusMenuLayout>();
	qDBusRegisterMetaType<DBusMenuIdList>();
	qDBusRegisterMetaType<DBusMenuItemProperties>();
	qDBusRegisterMetaType<DBusMenuItemPropertiesList>();
	qDBusRegisterMetaType<DBusMenuItemPropertyNames>();
	qDBusRegisterMetaType<DBusMenuItemPropertyNamesList>();

	this->interface = new DBusMenuInterface(service, path, QDBusConnection::sessionBus(), this);

	if (!this->interface->isValid()) {
		qCWarning(logDbusMenu).noquote() << "Cannot create DBusMenu for" << service << "at" << path;
		return;
	}

	QObject::connect(
	    this->interface,
	    &DBusMenuInterface::LayoutUpdated,
	    this,
	    &DBusMenu::onLayoutUpdated
	);

	this->properties.setInterface(this->interface);
	this->properties.updateAllViaGetAll();
}

void DBusMenu::prepareToShow(qint32 item, qint32 depth) {
	auto pending = this->interface->AboutToShow(item);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this, item, depth](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<bool> reply = *call;
		if (reply.isError()) {
			qCDebug(logDbusMenu) << "Error in AboutToShow, but showing anyway for menu" << item << "of"
			                     << this << reply.error();
		}

		this->updateLayout(item, depth);

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void DBusMenu::updateLayout(qint32 parent, qint32 depth) {
	auto pending = this->interface->GetLayout(parent, depth, QStringList());
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this, parent, depth](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<uint, DBusMenuLayout> reply = *call;

		if (reply.isError()) {
			qCWarning(logDbusMenu) << "Error updating layout for menu" << parent << "of" << this
			                       << reply.error();
		} else {
			auto layout = reply.argumentAt<1>();
			this->updateLayoutRecursive(layout, this->items.value(parent), depth);
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void DBusMenu::updateLayoutRecursive(
    const DBusMenuLayout& layout,
    DBusMenuItem* parent,
    qint32 depth
) {
	auto* item = this->items.value(layout.id);
	if (item == nullptr) {
		// there is an actual nullptr in the map and not no entry
		if (this->items.contains(layout.id)) {
			item = new DBusMenuItem(layout.id, this, parent);
			item->mShowChildren = parent != nullptr && parent->mShowChildren;
			this->items.insert(layout.id, item);
		}
	}

	if (item == nullptr) return;

	qCDebug(logDbusMenu) << "Updating layout recursively for" << this << "menu" << layout.id;
	item->updateProperties(layout.properties);

	if (depth != 0) {
		auto childrenChanged = false;
		auto iter = item->mChildren.begin();
		while (iter != item->mChildren.end()) {
			auto existing = std::ranges::find_if(layout.children, [&](const DBusMenuLayout& layout) {
				return layout.id == *iter;
			});

			if (!item->mShowChildren || existing == layout.children.end()) {
				qCDebug(logDbusMenu) << "Removing missing layout item" << this->items.value(*iter) << "from"
				                     << item;
				this->removeRecursive(*iter);
				iter = item->mChildren.erase(iter);
				childrenChanged = true;
			} else {
				iter++;
			}
		}

		for (const auto& child: layout.children) {
			if (item->mShowChildren && !item->mChildren.contains(child.id)) {
				qCDebug(logDbusMenu) << "Creating new layout item" << child.id << "in" << item;
				// item->mChildren.push_back(child.id);
				this->items.insert(child.id, nullptr);
				childrenChanged = true;
			}

			this->updateLayoutRecursive(child, item, depth - 1);
		}

		if (childrenChanged) {
			// reset to preserve order
			item->mChildren.clear();
			for (const auto& child: layout.children) {
				item->mChildren.push_back(child.id);
			}

			item->onChildrenUpdated();
		}
	}

	if (item->mShowChildren && !item->childrenLoaded) {
		item->childrenLoaded = true;
	}

	emit item->layoutUpdated();
}

void DBusMenu::removeRecursive(qint32 id) {
	auto* item = this->items.value(id);

	if (item != nullptr) {
		for (auto child: item->mChildren) {
			this->removeRecursive(child);
		}
	}

	this->items.remove(id);

	if (item != nullptr) {
		item->deleteLater();
	}
}

void DBusMenu::sendEvent(qint32 item, const QString& event) {
	qCDebug(logDbusMenu) << "Sending event" << event << "to menu" << item << "of" << this;

	auto pending =
	    this->interface->Event(item, event, QDBusVariant(0), QDateTime::currentSecsSinceEpoch());
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this, item, event](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<> reply = *call;

		if (reply.isError()) {
			qCWarning(logDbusMenu) << "Error sending event" << event << "to" << item << "of" << this
			                       << reply.error();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

DBusMenuItem* DBusMenu::menu() { return &this->rootItem; }

void DBusMenu::onLayoutUpdated(quint32 /*unused*/, qint32 parent) {
	// note: spec says this is recursive
	this->updateLayout(parent, -1);
}

void DBusMenu::onItemPropertiesUpdated( // NOLINT
    const DBusMenuItemPropertiesList& updatedProps,
    const DBusMenuItemPropertyNamesList& removedProps
) {
	for (const auto& propset: updatedProps) {
		auto* item = this->items.value(propset.id);
		if (item != nullptr) {
			item->updateProperties(propset.properties);
		}
	}

	for (const auto& propset: removedProps) {
		auto* item = this->items.value(propset.id);
		if (item != nullptr) {
			item->updateProperties({}, propset.properties);
		}
	}
}

QDebug operator<<(QDebug debug, DBusMenu* menu) {
	if (menu == nullptr) {
		debug << "DBusMenu(nullptr)";
		return debug;
	}

	auto saver = QDebugStateSaver(debug);
	debug.nospace() << "DBusMenu(" << static_cast<void*>(menu) << ", " << menu->properties.toString()
	                << ")";
	return debug;
}

QImage
DBusMenuPngImage::requestImage(const QString& /*unused*/, QSize* size, const QSize& /*unused*/) {
	auto image = QImage();

	if (!image.loadFromData(this->data, "PNG")) {
		qCWarning(logDbusMenu) << "Failed to load dbusmenu item png";
	}

	if (size != nullptr) *size = image.size();
	return image;
}

void DBusMenuHandle::setAddress(const QString& service, const QString& path) {
	if (service == this->service && path == this->path) return;
	this->service = service;
	this->path = path;
	this->onMenuPathChanged();
}

void DBusMenuHandle::refHandle() {
	this->refcount++;
	qCDebug(logDbusMenu) << this << "gained a reference. Refcount is now" << this->refcount;

	if (this->refcount == 1 || !this->mMenu) {
		this->onMenuPathChanged();
	} else {
		// Refresh the layout when opening a menu in case a bad client isn't updating it
		// and another ref is open somewhere.
		this->mMenu->rootItem.updateLayout();
	}
}

void DBusMenuHandle::unrefHandle() {
	this->refcount--;
	qCDebug(logDbusMenu) << this << "lost a reference. Refcount is now" << this->refcount;

	if (this->refcount == 0) {
		this->onMenuPathChanged();
	}
}

void DBusMenuHandle::onMenuPathChanged() {
	qCDebug(logDbusMenu) << "Updating" << this << "with refcount" << this->refcount;

	if (this->mMenu) {
		// Without this, layout updated can be sent after mMenu is set to null,
		// leaving loaded = true while mMenu = nullptr.
		QObject::disconnect(&this->mMenu->rootItem, nullptr, this, nullptr);
		this->mMenu->deleteLater();
		this->mMenu = nullptr;
		this->loaded = false;
		emit this->menuChanged();
	}

	if (this->refcount > 0 && !this->service.isEmpty() && !this->path.isEmpty()) {
		this->mMenu = new DBusMenu(this->service, this->path);
		this->mMenu->setParent(this);

		QObject::connect(&this->mMenu->rootItem, &DBusMenuItem::layoutUpdated, this, [this]() {
			QObject::disconnect(&this->mMenu->rootItem, &DBusMenuItem::layoutUpdated, this, nullptr);
			this->loaded = true;
			emit this->menuChanged();
		});

		this->mMenu->rootItem.setShowChildrenRecursive(true);
	}
}

QsMenuEntry* DBusMenuHandle::menu() { return this->loaded ? &this->mMenu->rootItem : nullptr; }

QDebug operator<<(QDebug debug, const DBusMenuHandle* handle) {
	if (handle) {
		auto saver = QDebugStateSaver(debug);
		debug.nospace() << "DBusMenuHandle(" << static_cast<const void*>(handle)
		                << ", service=" << handle->service << ", path=" << handle->path << ')';
	} else {
		debug << "DBusMenuHandle(nullptr)";
	}

	return debug;
}

} // namespace qs::dbus::dbusmenu
