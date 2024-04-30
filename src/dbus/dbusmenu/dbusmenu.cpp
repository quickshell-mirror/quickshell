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
#include "../../dbus/properties.hpp"
#include "dbus_menu.h"
#include "dbus_menu_types.hpp"

Q_LOGGING_CATEGORY(logDbusMenu, "quickshell.dbus.dbusmenu", QtWarningMsg);

namespace qs::dbus::dbusmenu {

DBusMenuItem::DBusMenuItem(qint32 id, DBusMenu* menu, DBusMenuItem* parentMenu)
    : QObject(menu)
    , id(id)
    , menu(menu)
    , parentMenu(parentMenu) {
	QObject::connect(
	    &this->menu->iconThemePath,
	    &AbstractDBusProperty::changed,
	    this,
	    &DBusMenuItem::iconChanged
	);
}

void DBusMenuItem::click() {
	if (this->displayChildren) {
		this->setShowChildren(!this->mShowChildren);
	} else {
		this->menu->sendEvent(this->id, "clicked");
	}
}

void DBusMenuItem::hover() const { this->menu->sendEvent(this->id, "hovered"); }

DBusMenu* DBusMenuItem::menuHandle() const { return this->menu; }
QString DBusMenuItem::label() const { return this->mLabel; }
QString DBusMenuItem::cleanLabel() const { return this->mCleanLabel; }
bool DBusMenuItem::enabled() const { return this->mEnabled; }

QString DBusMenuItem::icon() const {
	if (!this->iconName.isEmpty()) {
		return IconImageProvider::requestString(
		    this->iconName,
		    this->menu->iconThemePath.get().join(':')
		);
	} else if (this->image != nullptr) {
		return this->image->url();
	} else return nullptr;
}

ToggleButtonType::Enum DBusMenuItem::toggleType() const { return this->mToggleType; };
Qt::CheckState DBusMenuItem::checkState() const { return this->mCheckState; }
bool DBusMenuItem::isSeparator() const { return this->mSeparator; }

bool DBusMenuItem::isShowingChildren() const { return this->mShowChildren && this->childrenLoaded; }

void DBusMenuItem::setShowChildren(bool showChildren) {
	if (showChildren == this->mShowChildren) return;
	this->mShowChildren = showChildren;
	this->childrenLoaded = false;

	if (showChildren) {
		this->menu->prepareToShow(this->id, true);
	} else {
		this->menu->sendEvent(this->id, "closed");
		emit this->showingChildrenChanged();

		if (!this->mChildren.isEmpty()) {
			for (auto child: this->mChildren) {
				this->menu->removeRecursive(child);
			}

			this->mChildren.clear();
			this->onChildrenUpdated();
		}
	}
}

bool DBusMenuItem::hasChildren() const { return this->displayChildren; }

QQmlListProperty<DBusMenuItem> DBusMenuItem::children() {
	return QQmlListProperty<DBusMenuItem>(
	    this,
	    nullptr,
	    &DBusMenuItem::childrenCount,
	    &DBusMenuItem::childAt
	);
}

qsizetype DBusMenuItem::childrenCount(QQmlListProperty<DBusMenuItem>* property) {
	return reinterpret_cast<DBusMenuItem*>(property->object)->enabledChildren.count(); // NOLINT
}

DBusMenuItem* DBusMenuItem::childAt(QQmlListProperty<DBusMenuItem>* property, qsizetype index) {
	auto* item = reinterpret_cast<DBusMenuItem*>(property->object); // NOLINT
	return item->menu->items.value(item->enabledChildren.at(index));
}

void DBusMenuItem::updateProperties(const QVariantMap& properties, const QStringList& removed) {
	// Some programs appear to think sending an empty map does not mean "reset everything"
	// and instead means "do nothing". oh well...
	if (properties.isEmpty() && removed.isEmpty()) {
		qCDebug(logDbusMenu) << "Ignoring empty property update for" << this;
		return;
	}

	auto originalLabel = this->mLabel;
	//auto originalMnemonic = this->mnemonic;
	auto originalEnabled = this->mEnabled;
	auto originalVisible = this->visible;
	auto originalIconName = this->iconName;
	auto* originalImage = this->image;
	auto originalIsSeparator = this->mSeparator;
	auto originalToggleType = this->mToggleType;
	auto originalToggleState = this->mCheckState;
	auto originalDisplayChildren = this->displayChildren;

	auto label = properties.value("label");
	if (label.canConvert<QString>()) {
		auto text = label.value<QString>();
		this->mLabel = text;
		this->mCleanLabel = text;
		//this->mnemonic = QChar();

		for (auto i = 0; i < this->mLabel.length() - 1;) {
			if (this->mLabel.at(i) == '_') {
				//if (this->mnemonic == QChar()) this->mnemonic = this->mLabel.at(i + 1);
				this->mLabel.remove(i, 1);
				this->mLabel.insert(i + 1, "</u>");
				this->mLabel.insert(i, "<u>");
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
		this->mLabel = "";
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
			this->image = nullptr;
		} else if (this->image == nullptr || this->image->data != data) {
			this->image = new DBusMenuPngImage(data, this);
		}
	} else if (removed.isEmpty() || removed.contains("icon-data")) {
		this->image = nullptr;
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

		if (toggleTypeStr == "") this->mToggleType = ToggleButtonType::None;
		else if (toggleTypeStr == "checkmark") this->mToggleType = ToggleButtonType::CheckBox;
		else if (toggleTypeStr == "radio") this->mToggleType = ToggleButtonType::RadioButton;
		else {
			qCWarning(logDbusMenu) << "Unrecognized toggle type" << toggleTypeStr << "for" << this;
			this->mToggleType = ToggleButtonType::None;
		}
	} else if (removed.isEmpty() || removed.contains("toggle-type")) {
		this->mToggleType = ToggleButtonType::None;
	}

	auto toggleState = properties.value("toggle-state");
	if (toggleState.canConvert<qint32>()) {
		auto toggleStateInt = toggleState.value<qint32>();

		if (toggleStateInt == 0) this->mCheckState = Qt::Unchecked;
		else if (toggleStateInt == 1) this->mCheckState = Qt::Checked;
		else this->mCheckState = Qt::PartiallyChecked;
	} else if (removed.isEmpty() || removed.contains("toggle-state")) {
		this->mCheckState = Qt::PartiallyChecked;
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

	if (this->mLabel != originalLabel) emit this->labelChanged();
	//if (this->mnemonic != originalMnemonic) emit this->labelChanged();
	if (this->mEnabled != originalEnabled) emit this->enabledChanged();
	if (this->visible != originalVisible && this->parentMenu != nullptr)
		this->parentMenu->onChildrenUpdated();
	if (this->mToggleType != originalToggleType) emit this->toggleTypeChanged();
	if (this->mCheckState != originalToggleState) emit this->checkStateChanged();
	if (this->mSeparator != originalIsSeparator) emit this->separatorChanged();
	if (this->displayChildren != originalDisplayChildren) emit this->hasChildrenChanged();

	if (this->iconName != originalIconName || this->image != originalImage) {
		if (this->image != originalImage) {
			delete originalImage;
		}

		emit this->iconChanged();
	}

	qCDebug(logDbusMenu).nospace() << "Updated properties of " << this << " { label=" << this->mLabel
	                               << ", enabled=" << this->mEnabled << ", visible=" << this->visible
	                               << ", iconName=" << this->iconName << ", iconData=" << this->image
	                               << ", separator=" << this->mSeparator
	                               << ", toggleType=" << this->mToggleType
	                               << ", toggleState=" << this->mCheckState
	                               << ", displayChildren=" << this->displayChildren << " }";
}

void DBusMenuItem::onChildrenUpdated() {
	this->enabledChildren.clear();

	for (auto child: this->mChildren) {
		auto* item = this->menu->items.value(child);
		if (item->visible) this->enabledChildren.push_back(child);
	}

	emit this->childrenChanged();
}

QDebug operator<<(QDebug debug, DBusMenuItem* item) {
	if (item == nullptr) {
		debug << "DBusMenuItem(nullptr)";
		return debug;
	}

	auto saver = QDebugStateSaver(debug);
	debug.nospace() << "DBusMenuItem(" << static_cast<void*>(item) << ", id=" << item->id
	                << ", label=" << item->mLabel << ", menu=" << item->menu << ")";
	return debug;
}

QDebug operator<<(QDebug debug, const ToggleButtonType::Enum& toggleType) {
	auto saver = QDebugStateSaver(debug);
	debug.nospace() << "ToggleType::";

	switch (toggleType) {
	case ToggleButtonType::None: debug << "None"; break;
	case ToggleButtonType::CheckBox: debug << "Checkbox"; break;
	case ToggleButtonType::RadioButton: debug << "Radiobutton"; break;
	}

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

void DBusMenu::prepareToShow(qint32 item, bool sendOpened) {
	auto pending = this->interface->AboutToShow(item);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this, item, sendOpened](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<bool> reply = *call;
		if (reply.isError()) {
			qCWarning(logDbusMenu) << "Error in AboutToShow, but showing anyway for menu" << item << "of"
			                       << this << reply.error();
		}

		this->updateLayout(item, 1);
		if (sendOpened) this->sendEvent(item, "opened");

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
			auto existing = std::find_if(
			    layout.children.begin(),
			    layout.children.end(),
			    [&](const DBusMenuLayout& layout) { return layout.id == *iter; }
			);

			if (existing == layout.children.end()) {
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
				item->mChildren.push_back(child.id);
				this->items.insert(child.id, nullptr);
				childrenChanged = true;
			}

			this->updateLayoutRecursive(child, item, depth - 1);
		}

		if (childrenChanged) item->onChildrenUpdated();
	}

	if (item->mShowChildren && !item->childrenLoaded) {
		item->childrenLoaded = true;
		emit item->showingChildrenChanged();
	}
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

} // namespace qs::dbus::dbusmenu
