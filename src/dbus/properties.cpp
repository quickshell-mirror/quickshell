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
#include <qtversionchecks.h>
#include <qvariant.h>

#include "../core/logcat.hpp"
#include "dbus_properties.h"

QS_LOGGING_CATEGORY(logDbusProperties, "quickshell.dbus.properties", QtWarningMsg);

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
			*reinterpret_cast<QVariant*>(slot) = variant;
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

DBusPropertyGroup::DBusPropertyGroup(QVector<DBusPropertyCore*> properties, QObject* parent)
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

void DBusPropertyGroup::attachProperty(DBusPropertyCore* property) {
	this->properties.append(property);
}

void DBusPropertyGroup::updateAllDirect() {
	qCDebug(logDbusProperties).noquote()
	    << "Updating all properties of" << this->toString() << "via individual queries";

	if (this->interface == nullptr) {
		qFatal() << "Attempted to update properties of disconnected property group";
	}

	for (auto* property: this->properties) {
		this->requestPropertyUpdate(property);
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
			emit this->getAllFailed(reply.error());
		} else {
			qCDebug(logDbusProperties).noquote()
			    << "Received GetAll property set for" << this->toString();
			this->updatePropertySet(reply.value(), true);
			emit this->getAllFinished();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void DBusPropertyGroup::updatePropertySet(const QVariantMap& properties, bool complainMissing) {
	for (const auto [name, value]: properties.asKeyValueRange()) {
		auto prop = std::ranges::find_if(this->properties, [&name](DBusPropertyCore* prop) {
			return prop->nameRef() == name;
		});

		if (prop == this->properties.end()) {
			qCDebug(logDbusProperties) << "Ignoring untracked property update" << name << "for"
			                           << this->toString();
		} else {
			this->tryUpdateProperty(*prop, value);
		}
	}

	if (complainMissing) {
		for (const auto* prop: this->properties) {
			if (prop->isRequired() && !properties.contains(prop->name())) {
				qCWarning(logDbusProperties)
				    << prop->nameRef() << "missing from property set for" << this->toString();
			}
		}
	}
}

void DBusPropertyGroup::tryUpdateProperty(DBusPropertyCore* property, const QVariant& variant)
    const {
	property->mExists = true;

	auto error = property->store(variant);
	if (error.isValid()) {
		qCWarning(logDbusProperties).noquote()
		    << "Error demarshalling property update for" << this->propertyString(property);
		qCWarning(logDbusProperties) << error;
	} else {
		qCDebug(logDbusProperties).noquote()
		    << "Updated property" << this->propertyString(property) << "to" << property->valueString();
	}
}

void DBusPropertyGroup::requestPropertyUpdate(DBusPropertyCore* property) {
	const QString propStr = this->propertyString(property);

	if (this->interface == nullptr) {
		qFatal(logDbusProperties).noquote()
		    << "Tried to update property" << propStr << "of a disconnected interface";
	}

	qCDebug(logDbusProperties).noquote() << "Updating property" << propStr;

	auto pendingCall = this->propertyInterface->Get(this->interface->interface(), property->name());
	auto* call = new QDBusPendingCallWatcher(pendingCall, this);

	auto responseCallback = [this, propStr, property](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QDBusVariant> reply = *call;

		if (reply.isError()) {
			if (!property->isRequired() && reply.error().type() == QDBusError::InvalidArgs) {
				qCDebug(logDbusProperties) << "Error updating non-required property" << propStr;
				qCDebug(logDbusProperties) << reply.error();
			} else {
				qCWarning(logDbusProperties).noquote() << "Error updating property" << propStr;
				qCWarning(logDbusProperties) << reply.error();
			}
		} else {
			this->tryUpdateProperty(property, reply.value().variant());
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void DBusPropertyGroup::pushPropertyUpdate(DBusPropertyCore* property) {
	const QString propStr = this->propertyString(property);

	if (this->interface == nullptr) {
		qFatal(logDbusProperties).noquote()
		    << "Tried to write property" << propStr << "of a disconnected interface";
	}

	qCDebug(logDbusProperties).noquote() << "Writing property" << propStr;

	auto pendingCall = this->propertyInterface->Set(
	    this->interface->interface(),
	    property->name(),
	    QDBusVariant(property->serialize())
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

QString DBusPropertyGroup::toString() const {
	if (this->interface == nullptr) {
		return "{ DISCONNECTED }";
	} else {
		return this->interface->service() + this->interface->path() + "/"
		     + this->interface->interface();
	}
}

QString DBusPropertyGroup::propertyString(const DBusPropertyCore* property) const {
	return this->toString() % ':' % property->nameRef();
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
		auto prop = std::ranges::find_if(this->properties, [&name](DBusPropertyCore* prop) {
			return prop->nameRef() == name;
		});

		if (prop == this->properties.end()) {
			qCDebug(logDbusProperties) << "Ignoring untracked property invalidation" << name << "for"
			                           << this;
		} else {
			this->requestPropertyUpdate(*prop);
		}
	}

	this->updatePropertySet(changedProperties, false);
}

} // namespace qs::dbus

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
QDebug operator<<(QDebug debug, const QDBusObjectPath& path) {
	debug.nospace() << "QDBusObjectPath(" << path.path() << ")";
	return debug;
}
#endif
