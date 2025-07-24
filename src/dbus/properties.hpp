#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include <bit>
#include <qcontainerfwd.h>
#include <qdbusabstractinterface.h>
#include <qdbuserror.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbusreply.h>
#include <qdbusservicewatcher.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qoverload.h>
#include <qstringview.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtversionchecks.h>
#include <qvariant.h>

#include "../core/logcat.hpp"
#include "../core/util.hpp"

class DBusPropertiesInterface;

QS_DECLARE_LOGGING_CATEGORY(logDbusProperties);

namespace qs::dbus {

QDBusError demarshallVariant(const QVariant& variant, const QMetaType& type, void* slot);

template <typename T>
class DBusResult {
public:
	explicit DBusResult() = default;
	DBusResult(T value): value(std::move(value)) {}
	DBusResult(QDBusError error): error(std::move(error)) {}
	explicit DBusResult(T value, QDBusError error)
	    : value(std::move(value))
	    , error(std::move(error)) {}

	bool isValid() { return !this->error.isValid(); }

	T value {};
	QDBusError error;
};

template <typename T>
DBusResult<T> demarshallVariant(const QVariant& variant) {
	T value;
	auto error = demarshallVariant(variant, QMetaType::fromType<T>(), &value);
	return DBusResult<T>(value, error);
}

void asyncReadPropertyInternal(
    const QMetaType& type,
    QDBusAbstractInterface& interface,
    const QString& property,
    std::function<void(std::function<QDBusError(void*)>)> callback
);

template <typename T>
void asyncReadProperty(
    QDBusAbstractInterface& interface,
    const QString& property,
    const std::function<void(T, QDBusError)>& callback
) {
	asyncReadPropertyInternal(
	    QMetaType::fromType<T>(),
	    interface,
	    property,
	    [callback](std::function<QDBusError(void*)> internalCallback) { // NOLINT
		    T slot;
		    auto error = internalCallback(static_cast<void*>(&slot));
		    callback(slot, error);
	    }
	);
}

class DBusPropertyGroup;

class DBusPropertyCore {
public:
	DBusPropertyCore() = default;
	virtual ~DBusPropertyCore() = default;
	Q_DISABLE_COPY_MOVE(DBusPropertyCore);

	[[nodiscard]] virtual QString name() const = 0;
	[[nodiscard]] virtual QStringView nameRef() const = 0;
	[[nodiscard]] virtual QString valueString() = 0;
	[[nodiscard]] virtual bool isRequired() const = 0;
	[[nodiscard]] bool exists() const { return this->mExists; }

protected:
	virtual QDBusError store(const QVariant& variant) = 0;
	[[nodiscard]] virtual QVariant serialize() = 0;

private:
	bool mExists : 1 = false;

	friend class DBusPropertyGroup;
};

// Default implementation with no transformation
template <typename T>
struct DBusDataTransform {
	using Wire = T;
	using Data = T;
};

namespace bindable_p {

template <typename T>
struct BindableParams;

template <template <typename, typename, auto, auto> class B, typename C, typename T, auto O, auto S>
struct BindableParams<B<C, T, O, S>> {
	using Class = C;
	using Type = T;
	static constexpr size_t OFFSET = O;
};

template <typename Bindable>
struct BindableType {
	using Meta = BindableParams<Bindable>;
	using Type = Meta::Type;
};

template <typename T, typename = void>
struct HasFromWire: std::false_type {};

template <typename T>
struct HasFromWire<T, std::void_t<decltype(T::fromWire(std::declval<typename T::Wire>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct HasToWire: std::false_type {};

template <typename T>
struct HasToWire<T, std::void_t<decltype(T::toWire(std::declval<typename T::Data>()))>>
    : std::true_type {};

} // namespace bindable_p

template <
    typename T,
    auto offset,
    auto bindablePtr,
    auto updatedPtr,
    auto groupPtr,
    StringLiteral16 Name,
    bool required>
class DBusBindableProperty: public DBusPropertyCore {
	using PtrMeta = MemberPointerTraits<decltype(bindablePtr)>;
	using Owner = PtrMeta::Class;
	using Bindable = PtrMeta::Type;
	using BindableType = bindable_p::BindableType<Bindable>::Type;
	using BaseType = std::conditional_t<std::is_void_v<T>, BindableType, T>;
	using Transform = DBusDataTransform<BaseType>;
	using DataType = Transform::Data;
	using WireType = Transform::Wire;

public:
	explicit DBusBindableProperty() { this->group()->attachProperty(this); }

	[[nodiscard]] QString name() const override { return Name; }
	[[nodiscard]] QStringView nameRef() const override { return Name; }
	[[nodiscard]] bool isRequired() const override { return required; }

	[[nodiscard]] QString valueString() override {
		QString str;
		QDebug(&str) << this->bindable()->value();
		return str;
	}

	void write() { this->group()->pushPropertyUpdate(this); }
	void requestUpdate() { this->group()->requestPropertyUpdate(this); }

protected:
	QDBusError store(const QVariant& variant) override {
		DBusResult<DataType> result;

		if constexpr (bindable_p::HasFromWire<Transform>::value) {
			auto wireResult = demarshallVariant<WireType>(variant);
			if (!wireResult.isValid()) return wireResult.error;
			result = Transform::fromWire(std::move(wireResult.value));
		} else if constexpr (std::is_same_v<WireType, BaseType>) {
			result = demarshallVariant<DataType>(variant);
		} else {
			return QDBusError(QDBusError::NotSupported, "This property cannot be deserialized");
		}

		if (!result.isValid()) return result.error;
		this->bindable()->setValue(std::move(result.value));

		if constexpr (updatedPtr != nullptr) {
			(this->owner()->*updatedPtr)();
		}

		return QDBusError();
	}

	QVariant serialize() override {
		if constexpr (bindable_p::HasToWire<Transform>::value) {
			return QVariant::fromValue(Transform::toWire(this->bindable()->value()));
		} else if constexpr (std::is_same_v<WireType, BaseType>) {
			return QVariant::fromValue(this->bindable()->value());
		} else {
			return QVariant();
		}
	}

private:
	[[nodiscard]] constexpr Owner* owner() const {
		auto* self = std::bit_cast<char*>(this);
		return std::bit_cast<Owner*>(self - offset()); // NOLINT
	}

	[[nodiscard]] constexpr DBusPropertyGroup* group() const { return &(this->owner()->*groupPtr); }
	[[nodiscard]] constexpr Bindable* bindable() const { return &(this->owner()->*bindablePtr); }
};

class DBusPropertyGroup: public QObject {
	Q_OBJECT;

public:
	explicit DBusPropertyGroup(QVector<DBusPropertyCore*> properties = {}, QObject* parent = nullptr);
	explicit DBusPropertyGroup(QObject* parent): DBusPropertyGroup({}, parent) {}

	void setInterface(QDBusAbstractInterface* interface);
	void attachProperty(DBusPropertyCore* property);
	void updateAllDirect();
	void updateAllViaGetAll();
	void updatePropertySet(const QVariantMap& properties, bool complainMissing = true);
	[[nodiscard]] QString toString() const;
	[[nodiscard]] bool isConnected() const { return this->interface; }

	void pushPropertyUpdate(DBusPropertyCore* property);
	void requestPropertyUpdate(DBusPropertyCore* property);

signals:
	void getAllFinished();
	void getAllFailed(QDBusError error);

private slots:
	void onPropertiesChanged(
	    const QString& interfaceName,
	    const QVariantMap& changedProperties,
	    const QStringList& invalidatedProperties
	);

private:
	void tryUpdateProperty(DBusPropertyCore* property, const QVariant& variant) const;
	[[nodiscard]] QString propertyString(const DBusPropertyCore* property) const;

	DBusPropertiesInterface* propertyInterface = nullptr;
	QDBusAbstractInterface* interface = nullptr;
	QVector<DBusPropertyCore*> properties;

	friend class AbstractDBusProperty;
};

} // namespace qs::dbus

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
QDebug operator<<(QDebug debug, const QDBusObjectPath& path);
#endif

// NOLINTBEGIN
#define QS_DBUS_BINDABLE_PROPERTY_GROUP(Class, name) qs::dbus::DBusPropertyGroup name {this};

#define QS_DBUS_PROPERTY_BINDING_P(                                                                \
    Class,                                                                                         \
    Type,                                                                                          \
    property,                                                                                      \
    bindable,                                                                                      \
    updated,                                                                                       \
    group,                                                                                         \
    name,                                                                                          \
    required                                                                                       \
)                                                                                                  \
	static constexpr size_t _qs_property_##property##_offset() { return offsetof(Class, property); } \
                                                                                                   \
	qs::dbus::DBusBindableProperty<                                                                  \
	    Type,                                                                                        \
	    &Class::_qs_property_##property##_offset,                                                    \
	    &Class::bindable,                                                                            \
	    updated,                                                                                     \
	    &Class::group,                                                                               \
	    u##name,                                                                                     \
	    required>                                                                                    \
	    property;

#define QS_DBUS_PROPERTY_BINDING_8(                                                                \
    Class,                                                                                         \
    Type,                                                                                          \
    property,                                                                                      \
    bindable,                                                                                      \
    updated,                                                                                       \
    group,                                                                                         \
    name,                                                                                          \
    required                                                                                       \
)                                                                                                  \
	QS_DBUS_PROPERTY_BINDING_P(                                                                      \
	    Class,                                                                                       \
	    Type,                                                                                        \
	    property,                                                                                    \
	    bindable,                                                                                    \
	    &Class::updated,                                                                             \
	    group,                                                                                       \
	    name,                                                                                        \
	    required                                                                                     \
	)

#define QS_DBUS_PROPERTY_BINDING_7(Class, Type, property, bindable, group, name, required)         \
	QS_DBUS_PROPERTY_BINDING_P(Class, Type, property, bindable, nullptr, group, name, required)

#define QS_DBUS_PROPERTY_BINDING_6(Class, property, bindable, group, name, required)               \
	QS_DBUS_PROPERTY_BINDING_7(Class, void, property, bindable, group, name, required)

#define QS_DBUS_PROPERTY_BINDING_5(Class, property, bindable, group, name)                         \
	QS_DBUS_PROPERTY_BINDING_6(Class, property, bindable, group, name, true)

// Q_OBJECT_BINDABLE_PROPERTY elides the warning for the exact same reason,
// so we consider it safe to disable the warning.
// clang-format off
#define QS_DBUS_PROPERTY_BINDING(...) \
	QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
	QT_OVERLOADED_MACRO(QS_DBUS_PROPERTY_BINDING, __VA_ARGS__) \
	QT_WARNING_POP
// clang-format on
// NOLINTEND
