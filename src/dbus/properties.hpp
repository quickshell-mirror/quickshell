#pragma once

#include <functional>
#include <utility>

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
#include <qtmetamacros.h>
#include <qvariant.h>

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

	T value;
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

class AbstractDBusProperty: public QObject {
	Q_OBJECT;

public:
	explicit AbstractDBusProperty(
	    QString name,
	    const QMetaType& type,
	    bool required,
	    QObject* parent = nullptr
	)
	    : QObject(parent)
	    , name(std::move(name))
	    , type(type)
	    , required(required) {}

	[[nodiscard]] bool exists() const;
	[[nodiscard]] QString toString() const;
	[[nodiscard]] virtual QString valueString() = 0;

public slots:
	void update();
	void write();

signals:
	void changed();

protected:
	virtual QDBusError read(const QVariant& variant) = 0;
	virtual QVariant serialize() = 0;

private:
	void tryUpdate(const QVariant& variant);

	DBusPropertyGroup* group = nullptr;

	QString name;
	QMetaType type;
	bool required;
	bool mExists = false;

	friend class DBusPropertyGroup;
};

class DBusPropertyGroup: public QObject {
	Q_OBJECT;

public:
	explicit DBusPropertyGroup(
	    QVector<AbstractDBusProperty*> properties = QVector<AbstractDBusProperty*>(),
	    QObject* parent = nullptr
	);

	void setInterface(QDBusAbstractInterface* interface);
	void attachProperty(AbstractDBusProperty* property);
	void updateAllDirect();
	void updateAllViaGetAll();
	[[nodiscard]] QString toString() const;

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

	DBusPropertiesInterface* propertyInterface = nullptr;
	QDBusAbstractInterface* interface = nullptr;
	QVector<AbstractDBusProperty*> properties;

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
	    : AbstractDBusProperty(std::move(name), QMetaType::fromType<T>(), required, parent)
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
	QDBusError read(const QVariant& variant) override {
		auto result = demarshallVariant<T>(variant);

		if (result.isValid()) {
			this->set(std::move(result.value));
		}

		return result.error;
	}

	QVariant serialize() override { return QVariant::fromValue(this->value); }

private:
	T value;

	friend class DBusPropertyGroup;
};

} // namespace qs::dbus
