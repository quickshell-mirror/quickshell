#include "properties.hpp"
#include <algorithm>
#include <utility>

#include <qcontainerfwd.h>
#include <qdbusabstractinterface.h>
#include <qdbusargument.h>
#include <qdbuserror.h>
#include <qdbusextratypes.h>
#include <qdbusmessage.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmetatype.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qvariant.h>

#include "dbus_properties.h"

Q_LOGGING_CATEGORY(logDbusProperties, "quickshell.dbus.properties", QtWarningMsg);

namespace qs::dbus {

QDBusError demarshallVariant(const QVariant& variant, const QMetaType& type, void* slot) {
	const char* expectedSignature = "v";

	if (type.id() != QMetaType::QVariant) {
		expectedSignature = QDBusMetaType::typeToSignature(type);
		if (expectedSignature == nullptr) {
			qFatal() << "failed to demarshall unregistered dbus meta-type" << type << "with" << variant;
		}
	}

	if (variant.metaType() == type) {
		if (type.id() == QMetaType::QVariant) {
			*reinterpret_cast<QVariant*>(slot) = variant; // NOLINT
		} else {
			type.destruct(slot);
			type.construct(slot, variant.constData());
		}
	} else if (variant.metaType() == QMetaType::fromType<QDBusArgument>()) {
		auto arg = qvariant_cast<QDBusArgument>(variant);
		auto signature = arg.currentSignature();

		if (signature == expectedSignature) {
			if (!QDBusMetaType::demarshall(arg, type, slot)) {
				QString error;
				QDebug(&error) << "failed to deserialize dbus value" << variant << "into" << type;
				return QDBusError(QDBusError::InvalidArgs, error);
			}
		} else {
			QString error;
			QDebug(&error) << "mismatched signature while trying to demarshall" << variant << "into"
			               << type << "expected" << expectedSignature << "got" << signature;
			return QDBusError(QDBusError::InvalidArgs, error);
		}
	} else {
		QString error;
		QDebug(&error) << "failed to deserialize variant" << variant << "into" << type;
		return QDBusError(QDBusError::InvalidArgs, error);
	}

	return QDBusError();
}

void asyncReadPropertyInternal(
    const QMetaType& type,
    QDBusAbstractInterface& interface,
    const QString& property,
    std::function<void(std::function<QDBusError(void*)>)> callback // NOLINT
) {
	if (type.id() != QMetaType::QVariant) {
		const char* expectedSignature = QDBusMetaType::typeToSignature(type);
		if (expectedSignature == nullptr) {
			qFatal() << "qs::dbus::asyncReadPropertyInternal called with unregistered dbus meta-type"
			         << type;
		}
	}

	auto callMessage = QDBusMessage::createMethodCall(
	    interface.service(),
	    interface.path(),
	    "org.freedesktop.DBus.Properties",
	    "Get"
	);

	callMessage << interface.interface() << property;
	auto pendingCall = interface.connection().asyncCall(callMessage);

	auto* call = new QDBusPendingCallWatcher(pendingCall, &interface);

	auto responseCallback = [type, callback](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<QDBusVariant> reply = *call;

		callback([&](void* slot) {
			if (reply.isError()) {
				return reply.error();
			} else {
				return demarshallVariant(reply.value().variant(), type, slot);
			}
		});

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, &interface, responseCallback);
}

void AbstractDBusProperty::tryUpdate(const QVariant& variant) {
	this->mExists = true;

	auto error = this->read(variant);
	if (error.isValid()) {
		qCWarning(logDbusProperties).noquote()
		    << "Error demarshalling property update for" << this->toString();
		qCWarning(logDbusProperties) << error;
	} else {
		qCDebug(logDbusProperties).noquote()
		    << "Updated property" << this->toString() << "to" << this->valueString();
	}
}

void AbstractDBusProperty::update() {
	if (this->group == nullptr) {
		qFatal(logDbusProperties) << "Tried to update dbus property" << this->name
		                          << "which is not attached to a group";
	} else {
		const QString propStr = this->toString();

		if (this->group->interface == nullptr) {
			qFatal(logDbusProperties).noquote()
			    << "Tried to update property" << propStr << "of a disconnected interface";
		}

		qCDebug(logDbusProperties).noquote() << "Updating property" << propStr;

		auto pendingCall =
		    this->group->propertyInterface->Get(this->group->interface->interface(), this->name);

		auto* call = new QDBusPendingCallWatcher(pendingCall, this);

		auto responseCallback = [this, propStr](QDBusPendingCallWatcher* call) {
			const QDBusPendingReply<QDBusVariant> reply = *call;

			if (reply.isError()) {
				qCWarning(logDbusProperties).noquote() << "Error updating property" << propStr;
				qCWarning(logDbusProperties) << reply.error();
			} else {
				this->tryUpdate(reply.value().variant());
			}

			delete call;
		};

		QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
	}
}

void AbstractDBusProperty::write() {
	if (this->group == nullptr) {
		qFatal(logDbusProperties) << "Tried to write dbus property" << this->name
		                          << "which is not attached to a group";
	} else {
		const QString propStr = this->toString();

		if (this->group->interface == nullptr) {
			qFatal(logDbusProperties).noquote()
			    << "Tried to write property" << propStr << "of a disconnected interface";
		}

		qCDebug(logDbusProperties).noquote() << "Writing property" << propStr;

		auto pendingCall = this->group->propertyInterface->Set(
		    this->group->interface->interface(),
		    this->name,
		    QDBusVariant(this->serialize())
		);

		auto* call = new QDBusPendingCallWatcher(pendingCall, this);

		auto responseCallback = [propStr](QDBusPendingCallWatcher* call) {
			const QDBusPendingReply<> reply = *call;

			if (reply.isError()) {
				qCWarning(logDbusProperties).noquote() << "Error writing property" << propStr;
				qCWarning(logDbusProperties) << reply.error();
			}
			delete call;
		};

		QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
	}
}

bool AbstractDBusProperty::exists() const { return this->mExists; }

QString AbstractDBusProperty::toString() const {
	const QString group = this->group == nullptr ? "{ NO GROUP }" : this->group->toString();
	return group + ':' + this->name;
}

DBusPropertyGroup::DBusPropertyGroup(QVector<AbstractDBusProperty*> properties, QObject* parent)
    : QObject(parent)
    , properties(std::move(properties)) {}

void DBusPropertyGroup::setInterface(QDBusAbstractInterface* interface) {
	if (this->interface != nullptr) {
		delete this->propertyInterface;
		this->propertyInterface = nullptr;
	}

	if (interface != nullptr) {
		this->interface = interface;

		this->propertyInterface = new DBusPropertiesInterface(
		    interface->service(),
		    interface->path(),
		    interface->connection(),
		    this
		);

		QObject::connect(
		    this->propertyInterface,
		    &DBusPropertiesInterface::PropertiesChanged,
		    this,
		    &DBusPropertyGroup::onPropertiesChanged
		);
	}
}

void DBusPropertyGroup::attachProperty(AbstractDBusProperty* property) {
	this->properties.append(property);
	property->group = this;
}

void DBusPropertyGroup::updateAllDirect() {
	qCDebug(logDbusProperties).noquote()
	    << "Updating all properties of" << this->toString() << "via individual queries";

	if (this->interface == nullptr) {
		qFatal() << "Attempted to update properties of disconnected property group";
	}

	for (auto* property: this->properties) {
		property->update();
	}
}

void DBusPropertyGroup::updateAllViaGetAll() {
	qCDebug(logDbusProperties).noquote()
	    << "Updating all properties of" << this->toString() << "via GetAll";

	if (this->interface == nullptr) {
		qFatal() << "Attempted to update properties of disconnected property group";
	}

	auto pendingCall = this->propertyInterface->GetAll(this->interface->interface());
	auto* call = new QDBusPendingCallWatcher(pendingCall, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QVariantMap> reply = *call;

		if (reply.isError()) {
			qCWarning(logDbusProperties).noquote()
			    << "Error updating properties of" << this->toString() << "via GetAll";
			qCWarning(logDbusProperties) << reply.error();
		} else {
			qCDebug(logDbusProperties).noquote()
			    << "Received GetAll property set for" << this->toString();
			this->updatePropertySet(reply.value(), true);
		}

		delete call;
		emit this->getAllFinished();
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void DBusPropertyGroup::updatePropertySet(const QVariantMap& properties, bool complainMissing) {
	for (const auto [name, value]: properties.asKeyValueRange()) {
		auto prop = std::find_if(
		    this->properties.begin(),
		    this->properties.end(),
		    [&name](AbstractDBusProperty* prop) { return prop->name == name; }
		);

		if (prop == this->properties.end()) {
			qCDebug(logDbusProperties) << "Ignoring untracked property update" << name << "for"
			                           << this->toString();
		} else {
			(*prop)->tryUpdate(value);
		}
	}

	if (complainMissing) {
		for (const auto* prop: this->properties) {
			if (prop->required && !properties.contains(prop->name)) {
				qCWarning(logDbusProperties)
				    << prop->name << "missing from property set for" << this->toString();
			}
		}
	}
}

QString DBusPropertyGroup::toString() const {
	if (this->interface == nullptr) {
		return "{ DISCONNECTED }";
	} else {
		return this->interface->service() + this->interface->path() + "/"
		     + this->interface->interface();
	}
}

void DBusPropertyGroup::onPropertiesChanged(
    const QString& interfaceName,
    const QVariantMap& changedProperties,
    const QStringList& invalidatedProperties
) {
	if (interfaceName != this->interface->interface()) return;
	qCDebug(logDbusProperties).noquote()
	    << "Received property change set and invalidations for" << this->toString();

	for (const auto& name: invalidatedProperties) {
		auto prop = std::find_if(
		    this->properties.begin(),
		    this->properties.end(),
		    [&name](AbstractDBusProperty* prop) { return prop->name == name; }
		);

		if (prop == this->properties.end()) {
			qCDebug(logDbusProperties) << "Ignoring untracked property invalidation" << name << "for"
			                           << this;
		} else {
			(*prop)->update();
		}
	}

	this->updatePropertySet(changedProperties, false);
}

} // namespace qs::dbus
