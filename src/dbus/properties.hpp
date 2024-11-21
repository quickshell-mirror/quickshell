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
#include <qvariant.h>

#include "../core/util.hpp"

class DBusPropertiesInterface;

Q_DECLARE_LOGGING_CATEGORY(logDbusProperties);

namespace qs::dbus {

QDBusError demarshallVariant(const QVariant& variant, const QMetaType& type, void* slot);

template <typename T>
class DBusResult {
public:
	explicit DBusResult() = default;
	explicit DBusResult(T value): value(std::move(value)) {}
	explicit DBusResult(QDBusError error): error(std::move(error)) {}
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
    std::function<void(T, QDBusError)> callback
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

class AbstractDBusProperty
    : public QObject
    , public DBusPropertyCore {
	Q_OBJECT;

public:
	explicit AbstractDBusProperty(QString name, bool required, QObject* parent = nullptr)
	    : QObject(parent)
	    , required(required)
	    , mName(std::move(name)) {}

	[[nodiscard]] QString name() const override { return this->mName; };
	[[nodiscard]] QStringView nameRef() const override { return this->mName; };
	[[nodiscard]] bool isRequired() const override { return this->required; };

	[[nodiscard]] QString toString() const;

public slots:
	void update();
	void write();

signals:
	void changed();

private:
	bool required : 1;
	bool mExists : 1 = false;

	DBusPropertyGroup* group = nullptr;
	QString mName;

	friend class DBusPropertyGroup;
};

// Default implementation with no transformation
template <typename T>
struct DBusDataTransform {
	using Wire = T;
	using Data = T;

	static DBusResult<Data> fromWire(Wire&& wire) { return DBusResult<T>(std::move(wire)); }
	static Wire toWire(const Data& value) { return value; }
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

	[[nodiscard]] QString name() const override { return Name; };
	[[nodiscard]] QStringView nameRef() const override { return Name; };
	[[nodiscard]] bool isRequired() const override { return required; };

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

		if constexpr (std::is_same_v<WireType, BaseType>) {
			result = demarshallVariant<DataType>(variant);
		} else {
			auto wireResult = demarshallVariant<WireType>(variant);
			if (!wireResult.isValid()) return wireResult.error;
			result = Transform::fromWire(std::move(wireResult.value));
		}

		if (!result.isValid()) return result.error;
		this->bindable()->setValue(std::move(result.value));

		if constexpr (updatedPtr != nullptr) {
			(this->owner()->*updatedPtr)();
		}

		return QDBusError();
	}

	QVariant serialize() override {
		if constexpr (std::is_same_v<WireType, BaseType>) {
			return QVariant::fromValue(this->bindable()->value());
		} else {
			return QVariant::fromValue(Transform::toWire(this->bindable()->value()));
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
	void attachProperty(AbstractDBusProperty* property);
	void attachProperty(DBusPropertyCore* property);
	void updateAllDirect();
	void updateAllViaGetAll();
	[[nodiscard]] QString toString() const;

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
	void updatePropertySet(const QVariantMap& properties, bool complainMissing);
	void tryUpdateProperty(DBusPropertyCore* property, const QVariant& variant) const;
	[[nodiscard]] QString propertyString(const DBusPropertyCore* property) const;

	DBusPropertiesInterface* propertyInterface = nullptr;
	QDBusAbstractInterface* interface = nullptr;
	QVector<DBusPropertyCore*> properties;

	friend class AbstractDBusProperty;
};

template <typename T>
class DBusProperty: public AbstractDBusProperty {
public:
	explicit DBusProperty(
	    QString name,
	    T value = T(),
	    bool required = true,
	    QObject* parent = nullptr
	)
	    : AbstractDBusProperty(std::move(name), required, parent)
	    , value(std::move(value)) {}

	explicit DBusProperty(
	    DBusPropertyGroup& group,
	    QString name,
	    T value = T(),
	    bool required = true,
	    QObject* parent = nullptr
	)
	    : DBusProperty(std::move(name), std::move(value), required, parent) {
		group.attachProperty(this);
	}

	[[nodiscard]] QString valueString() override {
		QString str;
		QDebug(&str) << this->value;
		return str;
	}

	[[nodiscard]] const T& get() const { return this->value; }

	void set(T value) {
		this->value = std::move(value);
		emit this->changed();
	}

protected:
	QDBusError store(const QVariant& variant) override {
		auto result = demarshallVariant<T>(variant);

		if (result.isValid()) {
			this->set(std::move(result.value));
		}

		return result.error;
	}

	QVariant serialize() override { return QVariant::fromValue(this->value); }

private:
	T value;

	friend class DBusPropertyCore;
};

} // namespace qs::dbus

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
